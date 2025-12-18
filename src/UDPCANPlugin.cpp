/*******************************************************************************
** @file       UDPCANPlugin.cpp
** @author     The Open-Agriculture Developers
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "UDPCANPlugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/system_timing.hpp"
#include <cstring>
#include <chrono>

#ifndef _WIN32
#include <fcntl.h>
#include <errno.h>
#endif

namespace isobus
{
	UDPCANPlugin::UDPCANPlugin(const std::string &serverIP, int serverPort) :
	  serverIP(serverIP), serverPort(serverPort), socketFD(-1), isRunning(false), isOpen(false)
	{
#ifdef _WIN32
		int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (wsaResult != 0)
		{
			CANStackLogger::error("[UDP CAN Plugin] WSAStartup failed with error: " + std::to_string(wsaResult));
		}
#endif
	}

	UDPCANPlugin::~UDPCANPlugin()
	{
		close();
#ifdef _WIN32
		WSACleanup();
#endif
	}

	bool UDPCANPlugin::get_is_valid() const
	{
		return isOpen.load() && (socketFD >= 0);
	}

	void UDPCANPlugin::open()
	{
		if (isOpen.load())
			return;

		// Create UDP socket
#ifdef _WIN32
		socketFD = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
#else
		socketFD = socket(AF_INET, SOCK_DGRAM, 0);
#endif

		if (socketFD < 0)
		{
			CANStackLogger::error("[UDP CAN Plugin] Failed to create socket");
			return;
		}

#ifdef _WIN32
		// Set socket to non-blocking mode on Windows
		u_long mode = 1;
		ioctlsocket(socketFD, FIONBIO, &mode);
#else
		// Set socket to non-blocking mode on Unix
		int flags = fcntl(socketFD, F_GETFL, 0);
		fcntl(socketFD, F_SETFL, flags | O_NONBLOCK);
#endif

		// Configure server address
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(static_cast<uint16_t>(serverPort));
		inet_pton(AF_INET, serverIP.c_str(), &serverAddress.sin_addr);

		isOpen.store(true);
		isRunning.store(true);

		// Start receive thread
		receiveThread = std::thread(&UDPCANPlugin::receive_thread_function, this);

		CANStackLogger::info("[UDP CAN Plugin] Connected to " + serverIP + ":" + std::to_string(serverPort));
	}

	void UDPCANPlugin::close()
	{
		if (!isOpen.load())
			return;

		isRunning.store(false);
		isOpen.store(false);

		if (receiveThread.joinable())
			receiveThread.join();

#ifdef _WIN32
		closesocket(socketFD);
#else
		::close(socketFD);
#endif
		socketFD = -1;

		CANStackLogger::info("[UDP CAN Plugin] Disconnected");
	}

	bool UDPCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
	{
		if (!get_is_valid())
			return false;

		// Validate data length to prevent buffer overflow
		if (canFrame.dataLength > 8)
		{
			CANStackLogger::error("[UDP CAN Plugin] Invalid CAN frame data length: " + std::to_string(canFrame.dataLength));
			return false;
		}

		// Cannelloni format: [ID (4 bytes BE)] [DLC (1 byte)] [Data (0-8 bytes)]
		uint8_t buffer[13];

		// CAN ID (Big Endian)
		buffer[0] = (canFrame.identifier >> 24) & 0xFF;
		buffer[1] = (canFrame.identifier >> 16) & 0xFF;
		buffer[2] = (canFrame.identifier >> 8) & 0xFF;
		buffer[3] = canFrame.identifier & 0xFF;

		// DLC
		buffer[4] = canFrame.dataLength;

		// Data
		memcpy(&buffer[5], canFrame.data, canFrame.dataLength);

		int totalLength = 5 + canFrame.dataLength;
		int sentBytes = sendto(socketFD, reinterpret_cast<const char *>(buffer), totalLength, 0,
		                       reinterpret_cast<struct sockaddr *>(&serverAddress), sizeof(serverAddress));

		if (sentBytes != totalLength)
		{
			CANStackLogger::warn("[UDP CAN Plugin] Failed to send frame");
			return false;
		}

		return true;
	}

	bool UDPCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
	{
		std::lock_guard<std::mutex> lock(queueMutex);

		if (!receiveQueue.empty())
		{
			canFrame = receiveQueue.front();
			receiveQueue.pop();
			return true;
		}

		return false;
	}

	void UDPCANPlugin::receive_thread_function()
	{
		uint8_t buffer[13];
		struct sockaddr_in senderAddress;
		socklen_t senderAddressLen = sizeof(senderAddress);

		while (isRunning.load())
		{
			int receivedBytes = recvfrom(socketFD, reinterpret_cast<char *>(buffer), sizeof(buffer), 0,
			                             reinterpret_cast<struct sockaddr *>(&senderAddress), &senderAddressLen);

			if (receivedBytes >= 5)
			{
				isobus::CANMessageFrame frame;

				// Parse CAN ID (Big Endian)
				frame.identifier = (static_cast<uint32_t>(buffer[0]) << 24) |
				                   (static_cast<uint32_t>(buffer[1]) << 16) |
				                   (static_cast<uint32_t>(buffer[2]) << 8) |
				                   (static_cast<uint32_t>(buffer[3]));

				// Parse DLC
				frame.dataLength = buffer[4];
				if (frame.dataLength > 8)
					frame.dataLength = 8;

				// Validate we received enough bytes for the claimed data length
				if (receivedBytes < 5 + frame.dataLength)
				{
					CANStackLogger::warn("[UDP CAN Plugin] Received incomplete frame, discarding");
					continue;
				}

				// Parse Data
				memcpy(frame.data, &buffer[5], frame.dataLength);

				frame.isExtendedFrame = (frame.identifier > 0x7FF);
				frame.channel = 0;
				frame.timestamp_us = isobus::SystemTiming::get_timestamp_us();

				// Push frame to queue
				{
					std::lock_guard<std::mutex> lock(queueMutex);
					receiveQueue.push(frame);
				}
			}
			else if (receivedBytes < 0)
			{
#ifdef _WIN32
				int error = WSAGetLastError();
				if (error != WSAEWOULDBLOCK)
				{
					// Real error occurred
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				else
				{
					// No data available, sleep briefly
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
#else
				if (errno != EAGAIN && errno != EWOULDBLOCK)
				{
					// Real error occurred
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
				else
				{
					// No data available, sleep briefly
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
#endif
			}
		}
	}

	void UDPCANPlugin::set_server_ip(const std::string &ip)
	{
		serverIP = ip;
	}

	void UDPCANPlugin::set_server_port(int port)
	{
		serverPort = port;
	}

	std::string UDPCANPlugin::get_server_ip() const
	{
		return serverIP;
	}

	int UDPCANPlugin::get_server_port() const
	{
		return serverPort;
	}
}
