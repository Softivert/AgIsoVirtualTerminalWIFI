#pragma once

// On Windows, include winsock2.h before any JUCE headers to avoid conflicts
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "JuceHeader.h"
#include <string>

class Settings
{
public:
	Settings();
	~Settings();

	bool load_settings();
	std::shared_ptr<ValueTree> settingsValueTree();
	int vt_number() const;
	std::string udp_server_ip() const;
	int udp_server_port() const;

private:
	std::shared_ptr<ValueTree> m_settings;
	int m_vtNumber = 1;
	std::string m_udpServerIP = "192.168.1.100";
	int m_udpServerPort = 20000;
};
