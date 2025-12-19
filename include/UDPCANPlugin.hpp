//================================================================================================
/// @file UDPCANPlugin.hpp
///
/// @brief Defines a UDP-based CAN plugin compatible with cannelloni for remote CAN connectivity.
/// @author The Open-Agriculture Developers
///
/// @copyright 2023 The Open-Agriculture Contributors
//================================================================================================
#ifndef UDP_CAN_PLUGIN_HPP
#define UDP_CAN_PLUGIN_HPP

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
// Undefine Windows macros that conflict with JUCE
#ifdef Rectangle
#undef Rectangle
#endif
#ifdef MessageBox
#undef MessageBox
#endif
#endif

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include <thread>
#include <atomic>
#include <string>
#include <queue>
#include <mutex>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace isobus
{
	/// @brief Plugin for UDP-based CAN communication (cannelloni compatible)
	class UDPCANPlugin : public CANHardwarePlugin
	{
	public:
		/// @brief Constructor
		/// @param serverIP IP address of the cannelloni server (e.g., Raspberry Pi)
		/// @param serverPort UDP port (default: 20000 for cannelloni)
		UDPCANPlugin(const std::string &serverIP, int serverPort = 20000);

		/// @brief Destructor
		virtual ~UDPCANPlugin();

		/// @brief Returns if the connection is valid
		bool get_is_valid() const override;

		/// @brief Closes the connection
		void close() override;

		/// @brief Connects to the UDP server
		void open() override;

		/// @brief Writes a frame to the UDP socket
		bool write_frame(const isobus::CANMessageFrame &canFrame) override;

		/// @brief Reads frames from UDP (called periodically)
		bool read_frame(isobus::CANMessageFrame &canFrame) override;

		/// @brief Set the server IP address
		void set_server_ip(const std::string &ip);

		/// @brief Set the server port
		void set_server_port(int port);

		/// @brief Get the server IP address
		std::string get_server_ip() const;

		/// @brief Get the server port
		int get_server_port() const;

		/// @brief Returns the number of channels (always 1 for UDP)
		std::uint8_t get_number_of_channels() const override;

	private:
		void receive_thread_function();

		std::string serverIP;
		int serverPort;
		int socketFD;
		std::atomic<bool> isRunning;
		std::atomic<bool> isOpen;
		std::thread receiveThread;
		std::queue<isobus::CANMessageFrame> receiveQueue;
		std::mutex queueMutex;

		struct sockaddr_in serverAddress;

#ifdef _WIN32
		WSADATA wsaData;
#endif
	};
}

#endif // UDP_CAN_PLUGIN_HPP
