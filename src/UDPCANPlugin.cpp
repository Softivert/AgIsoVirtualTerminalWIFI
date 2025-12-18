//================================================================================================
/// @file UDPCANPlugin.cpp
///
/// @brief Implementation of the UDP CAN plugin for cannelloni protocol
/// @author Open-Agriculture Contributors
///
/// @copyright 2024 The Open-Agriculture Contributors
//================================================================================================
#include "UDPCANPlugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"

#include <cstring>

UDPCANPlugin::UDPCANPlugin(const std::string &serverIP, int serverPort) :
  serverIP(serverIP),
  serverPort(serverPort),
  running(false)
{
}

UDPCANPlugin::~UDPCANPlugin()
{
	close();
}

bool UDPCANPlugin::open()
{
	if (running.load())
	{
		isobus::CANStackLogger::warn("[UDP CAN] Plugin is already open.");
		return true;
	}

	socket = std::make_unique<juce::DatagramSocket>();
	
	// Bind to any local port for receiving
	if (!socket->bindToPort(0))
	{
		isobus::CANStackLogger::error("[UDP CAN] Failed to bind socket to local port.");
		socket.reset();
		return false;
	}

	running.store(true);
	
	// Start the receive thread
	receiveThread = std::make_unique<std::thread>(&UDPCANPlugin::receive_thread_function, this);

	isobus::CANStackLogger::info("[UDP CAN] Plugin opened successfully. Target: " + serverIP + ":" + std::to_string(serverPort));
	return true;
}

bool UDPCANPlugin::close()
{
	if (!running.load())
	{
		return true;
	}

	running.store(false);

	// Wait for the receive thread to finish
	if (receiveThread && receiveThread->joinable())
	{
		receiveThread->join();
	}

	socket.reset();
	
	juce::ScopedLock lock(frameLock);
	receiveQueue.clear();

	isobus::CANStackLogger::info("[UDP CAN] Plugin closed.");
	return true;
}

bool UDPCANPlugin::get_is_valid() const
{
	return running.load() && socket != nullptr;
}

bool UDPCANPlugin::read_frame(isobus::CANMessageFrame &canFrame)
{
	juce::ScopedLock lock(frameLock);
	
	if (receiveQueue.empty())
	{
		return false;
	}

	canFrame = receiveQueue.front();
	receiveQueue.erase(receiveQueue.begin());
	return true;
}

bool UDPCANPlugin::write_frame(const isobus::CANMessageFrame &canFrame)
{
	if (!get_is_valid())
	{
		return false;
	}

	CanFrame frame;
	std::memset(&frame, 0, sizeof(CanFrame));

	// Convert isobus::CANMessageFrame to CanFrame
	frame.can_id = canFrame.identifier;
	
	// Set extended frame flag if needed
	if (canFrame.isExtendedFrame)
	{
		frame.can_id |= 0x80000000; // CAN_EFF_FLAG
	}

	frame.can_dlc = canFrame.dataLength;
	
	if (canFrame.dataLength > 8)
	{
		isobus::CANStackLogger::warn("[UDP CAN] Frame data length exceeds 8 bytes, truncating.");
		frame.can_dlc = 8;
	}

	std::memcpy(frame.data, canFrame.data, frame.can_dlc);

	// Send the frame via UDP
	int bytesSent = socket->write(serverIP, serverPort, &frame, sizeof(CanFrame));
	
	if (bytesSent != sizeof(CanFrame))
	{
		isobus::CANStackLogger::error("[UDP CAN] Failed to send frame. Bytes sent: " + std::to_string(bytesSent));
		return false;
	}

	return true;
}

void UDPCANPlugin::reconfigure(const std::string &serverIP, int serverPort)
{
	bool wasRunning = running.load();
	
	if (wasRunning)
	{
		close();
	}

	this->serverIP = serverIP;
	this->serverPort = serverPort;

	if (wasRunning)
	{
		open();
	}
}

std::string UDPCANPlugin::get_server_ip() const
{
	return serverIP;
}

int UDPCANPlugin::get_server_port() const
{
	return serverPort;
}

void UDPCANPlugin::receive_thread_function()
{
	isobus::CANStackLogger::info("[UDP CAN] Receive thread started.");

	char buffer[sizeof(CanFrame)];
	
	while (running.load())
	{
		// Use a timeout to allow checking the running flag periodically
		int ready = socket->waitUntilReady(true, 100); // 100ms timeout
		
		if (ready < 0)
		{
			isobus::CANStackLogger::error("[UDP CAN] Socket error in receive thread.");
			break;
		}
		
		if (ready == 0)
		{
			// Timeout, continue to check running flag
			continue;
		}

		juce::String senderIP;
		int senderPort = 0;
		int bytesRead = socket->read(buffer, sizeof(CanFrame), false, senderIP, senderPort);

		if (bytesRead == sizeof(CanFrame))
		{
			CanFrame *frame = reinterpret_cast<CanFrame *>(buffer);
			
			isobus::CANMessageFrame canFrame;
			canFrame.identifier = frame->can_id & 0x1FFFFFFF; // Mask out flags
			canFrame.isExtendedFrame = (frame->can_id & 0x80000000) != 0; // CAN_EFF_FLAG
			canFrame.dataLength = frame->can_dlc;
			
			if (canFrame.dataLength > 8)
			{
				canFrame.dataLength = 8;
			}

			std::memcpy(canFrame.data, frame->data, canFrame.dataLength);
			canFrame.timestamp_us = juce::Time::currentTimeMillis() * 1000; // Convert to microseconds

			juce::ScopedLock lock(frameLock);
			receiveQueue.push_back(canFrame);
		}
		else if (bytesRead > 0)
		{
			isobus::CANStackLogger::warn("[UDP CAN] Received incomplete frame. Bytes: " + std::to_string(bytesRead));
		}
	}

	isobus::CANStackLogger::info("[UDP CAN] Receive thread stopped.");
}
