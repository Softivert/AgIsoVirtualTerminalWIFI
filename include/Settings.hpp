#pragma once

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
	
	// UDP CAN settings
	std::string udp_server_ip() const;
	void set_udp_server_ip(const std::string &ip);
	int udp_server_port() const;
	void set_udp_server_port(int port);

private:
	std::shared_ptr<ValueTree> m_settings;
	int m_vtNumber = 1;
	std::string m_udpServerIP = "192.168.1.100";
	int m_udpServerPort = 20000;
};
