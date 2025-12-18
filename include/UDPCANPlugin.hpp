//================================================================================================
/// @file UDPCANPlugin.hpp
///
/// @brief A CAN hardware plugin for UDP/TCP communication using cannelloni protocol
/// @author Open-Agriculture Contributors
///
/// @copyright 2024 The Open-Agriculture Contributors
//================================================================================================
#ifndef UDP_CAN_PLUGIN_HPP
#define UDP_CAN_PLUGIN_HPP

#include "isobus/hardware_integration/can_hardware_plugin.hpp"
#include "isobus/isobus/can_hardware_abstraction.hpp"
#include "isobus/isobus/can_message_frame.hpp"

#include <JuceHeader.h>
#include <atomic>
#include <memory>
#include <string>
#include <thread>

/// @brief A CAN driver for UDP communication compatible with cannelloni
class UDPCANPlugin : public isobus::CANHardwarePlugin
{
public:
	/// @brief Constructor for the UDP CAN plugin
	/// @param serverIP The IP address of the cannelloni server
	/// @param serverPort The UDP port of the cannelloni server
	UDPCANPlugin(const std::string &serverIP, int serverPort);

	/// @brief Destructor
	virtual ~UDPCANPlugin();

	/// @brief Opens the UDP connection
	/// @return True if the connection was successful
	bool open() override;

	/// @brief Closes the UDP connection
	/// @return True if the connection was closed successfully
	bool close() override;

	/// @brief Returns if the connection is valid
	/// @return True if the UDP socket is connected
	bool get_is_valid() const override;

	/// @brief Reads a CAN frame from the UDP socket
	/// @param[out] canFrame The CAN frame that was read
	/// @return True if a frame was read successfully
	bool read_frame(isobus::CANMessageFrame &canFrame) override;

	/// @brief Writes a CAN frame to the UDP socket
	/// @param[in] canFrame The CAN frame to write
	/// @return True if the frame was written successfully
	bool write_frame(const isobus::CANMessageFrame &canFrame) override;

	/// @brief Reconfigure the plugin with new IP and port
	/// @param serverIP The new IP address
	/// @param serverPort The new port
	void reconfigure(const std::string &serverIP, int serverPort);

	/// @brief Get the configured server IP
	/// @return The server IP address
	std::string get_server_ip() const;

	/// @brief Get the configured server port
	/// @return The server port
	int get_server_port() const;

private:
	/// @brief Structure representing a SocketCAN frame (compatible with cannelloni)
	struct CanFrame
	{
		std::uint32_t can_id; ///< CAN identifier (with EFF/RTR/ERR flags)
		std::uint8_t can_dlc; ///< Data length code
		std::uint8_t __pad;   ///< Padding
		std::uint8_t __res0;  ///< Reserved
		std::uint8_t __res1;  ///< Reserved
		std::uint8_t data[8]; ///< CAN frame data
	} __attribute__((packed));

	/// @brief Thread function for receiving frames
	void receive_thread_function();

	std::unique_ptr<juce::DatagramSocket> socket; ///< UDP socket for communication
	std::string serverIP;                         ///< IP address of the cannelloni server
	int serverPort;                               ///< UDP port of the cannelloni server
	std::atomic<bool> running;                    ///< Flag indicating if the plugin is running
	std::unique_ptr<std::thread> receiveThread;   ///< Thread for receiving frames
	juce::CriticalSection frameLock;              ///< Lock for thread-safe frame access
	std::vector<isobus::CANMessageFrame> receiveQueue; ///< Queue of received frames
};

#endif // UDP_CAN_PLUGIN_HPP
