/*******************************************************************************
** @file       ConfigureHardwareComponent.cpp
** @author     Adrian Del Grosso
** @copyright  The Open-Agriculture Developers
*******************************************************************************/
#include "ConfigureHardwareComponent.hpp"

#include "ConfigureHardwareWindow.hpp"
#include "ServerMainComponent.hpp"
#include "UDPCANPlugin.hpp"
#include "isobus/isobus/can_stack_logger.hpp"
#include "isobus/utility/to_string.hpp"

#ifdef JUCE_WINDOWS
#include "isobus/hardware_integration/toucan_vscp_canal.hpp"
#elif JUCE_LINUX
#include "isobus/hardware_integration/socket_can_interface.hpp"
#endif

ConfigureHardwareComponent::ConfigureHardwareComponent(ConfigureHardwareWindow &parent, std::vector<std::shared_ptr<isobus::CANHardwarePlugin>> &canDrivers) :
  okButton("OK"),
  parentCANDrivers(canDrivers)
{
	setSize(400, 380);
	okButton.setSize(100, 30);
	okButton.setTopLeftPosition(getWidth() / 2 - okButton.getWidth() / 2, 320);
	addAndMakeVisible(okButton);

	// Create input filters once for reuse
	auto numericInputFilter = new TextEditor::LengthAndCharacterRestriction(10, "1234567890");
	auto portInputFilter = new TextEditor::LengthAndCharacterRestriction(5, "1234567890");

#ifdef JUCE_WINDOWS
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");

#ifdef ISOBUS_WINDOWSINNOMAKERUSB2CAN_AVAILABLE
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN", "TouCAN", "SysTec", "UDP CAN" }, 1);
#else
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "Innomaker2CAN (not supported with mingw)", "TouCAN", "SysTec", "UDP CAN" }, 1);
#endif
	int selectedID = 1;

	for (std::uint8_t i = 0; i < parentCANDrivers.size(); i++)
	{
		if ((nullptr != parentCANDrivers.at(i)) &&
		    (parentCANDrivers.at(i) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0)))
		{
			selectedID = i + 1;
			break;
		}
	}
	hardwareInterfaceSelector.setSelectedId(selectedID);
	hardwareInterfaceSelector.setSize(getWidth() - 20, 30);
	hardwareInterfaceSelector.setTopLeftPosition(10, 80);
	hardwareInterfaceSelector.onChange = [this]() {
		if (3 == hardwareInterfaceSelector.getSelectedId())
		{
			touCANSerialEditor.setVisible(true);
			udpServerIPEditor.setVisible(false);
			udpServerPortEditor.setVisible(false);
		}
		else if (5 == hardwareInterfaceSelector.getSelectedId())
		{
			touCANSerialEditor.setVisible(false);
			udpServerIPEditor.setVisible(true);
			udpServerPortEditor.setVisible(true);
		}
		else
		{
			touCANSerialEditor.setVisible(false);
			udpServerIPEditor.setVisible(false);
			udpServerPortEditor.setVisible(false);
		}
		repaint();
	};
	addAndMakeVisible(hardwareInterfaceSelector);

	touCANSerialEditor.setName("TouCAN Serial Number");
	touCANSerialEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(2))->get_serial_number()));
	touCANSerialEditor.setSize(getWidth() - 20, 30);
	touCANSerialEditor.setTopLeftPosition(10, 140);
	touCANSerialEditor.setInputFilter(numericInputFilter, true);
	addChildComponent(touCANSerialEditor);

	// UDP Server IP Editor
	udpServerIPEditor.setName("UDP Server IP");
	if (parentCANDrivers.size() > 4)
	{
		udpServerIPEditor.setText(std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(4))->get_server_ip());
	}
	udpServerIPEditor.setSize(getWidth() - 20, 30);
	udpServerIPEditor.setTopLeftPosition(10, 140);
	addChildComponent(udpServerIPEditor);

	// UDP Server Port Editor
	udpServerPortEditor.setName("UDP Server Port");
	if (parentCANDrivers.size() > 4)
	{
		udpServerPortEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(4))->get_server_port()));
	}
	udpServerPortEditor.setSize(getWidth() - 20, 30);
	udpServerPortEditor.setTopLeftPosition(10, 200);
	udpServerPortEditor.setInputFilter(portInputFilter, true);
	addChildComponent(udpServerPortEditor);
#elif JUCE_LINUX
	socketCANNameEditor.setName("SocketCAN Interface Name");
	socketCANNameEditor.setText(std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->get_device_name());
	socketCANNameEditor.setSize(getWidth() - 20, 30);
	socketCANNameEditor.setTopLeftPosition(10, 80);
	addAndMakeVisible(socketCANNameEditor);

	// UDP Server IP Editor
	udpServerIPEditor.setName("UDP Server IP");
	if (parentCANDrivers.size() > 1)
	{
		udpServerIPEditor.setText(std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->get_server_ip());
	}
	udpServerIPEditor.setSize(getWidth() - 20, 30);
	udpServerIPEditor.setTopLeftPosition(10, 140);
	addAndMakeVisible(udpServerIPEditor);

	// UDP Server Port Editor
	udpServerPortEditor.setName("UDP Server Port");
	if (parentCANDrivers.size() > 1)
	{
		udpServerPortEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->get_server_port()));
	}
	udpServerPortEditor.setSize(getWidth() - 20, 30);
	udpServerPortEditor.setTopLeftPosition(10, 200);
	udpServerPortEditor.setInputFilter(portInputFilter, true);
	addAndMakeVisible(udpServerPortEditor);
#elif defined(JUCE_MAC)
	hardwareInterfaceSelector.setName("Hardware Interface");
	hardwareInterfaceSelector.setTextWhenNothingSelected("Select Hardware Interface");
	hardwareInterfaceSelector.addItemList({ "PEAK PCAN USB", "UDP CAN" }, 1);
	
	int selectedID = 1;
	for (std::uint8_t i = 0; i < parentCANDrivers.size(); i++)
	{
		if ((nullptr != parentCANDrivers.at(i)) &&
		    (parentCANDrivers.at(i) == isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0)))
		{
			selectedID = i + 1;
			break;
		}
	}
	hardwareInterfaceSelector.setSelectedId(selectedID);
	hardwareInterfaceSelector.setSize(getWidth() - 20, 30);
	hardwareInterfaceSelector.setTopLeftPosition(10, 80);
	hardwareInterfaceSelector.onChange = [this]() {
		if (2 == hardwareInterfaceSelector.getSelectedId())
		{
			udpServerIPEditor.setVisible(true);
			udpServerPortEditor.setVisible(true);
		}
		else
		{
			udpServerIPEditor.setVisible(false);
			udpServerPortEditor.setVisible(false);
		}
		repaint();
	};
	addAndMakeVisible(hardwareInterfaceSelector);

	// UDP Server IP Editor
	udpServerIPEditor.setName("UDP Server IP");
	if (parentCANDrivers.size() > 1)
	{
		udpServerIPEditor.setText(std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->get_server_ip());
	}
	udpServerIPEditor.setSize(getWidth() - 20, 30);
	udpServerIPEditor.setTopLeftPosition(10, 140);
	addChildComponent(udpServerIPEditor);

	// UDP Server Port Editor
	udpServerPortEditor.setName("UDP Server Port");
	if (parentCANDrivers.size() > 1)
	{
		udpServerPortEditor.setText(isobus::to_string(std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->get_server_port()));
	}
	udpServerPortEditor.setSize(getWidth() - 20, 30);
	udpServerPortEditor.setTopLeftPosition(10, 200);
	udpServerPortEditor.setInputFilter(portInputFilter, true);
	addChildComponent(udpServerPortEditor);
#endif
	okButton.onClick = [this, &parent]() {
		parent.setVisible(false);

#ifdef JUCE_WINDOWS
		if (3 == hardwareInterfaceSelector.getSelectedId()) // TouCAN
		{
			int serial = touCANSerialEditor.getText().trim().getIntValue();
			std::static_pointer_cast<isobus::TouCANPlugin>(parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1))->reconfigure(0, static_cast<std::uint32_t>(serial));
		}

		// Update UDP settings
		if (parentCANDrivers.size() > 4)
		{
			std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(4))->set_server_ip(udpServerIPEditor.getText().toStdString());
			int port = udpServerPortEditor.getText().trim().getIntValue();
			if (port > 0 && port <= 65535)
			{
				std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(4))->set_server_port(port);
			}
			isobus::CANStackLogger::info("Updated UDP CAN settings: " + udpServerIPEditor.getText().toStdString() + ":" + udpServerPortEditor.getText().toStdString());
		}

		if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
		{
			isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
		}
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1));
		isobus::CANStackLogger::info("Updated assigned CAN driver.");
#elif defined(JUCE_MAC)
		// Update UDP settings
		if (parentCANDrivers.size() > 1)
		{
			std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->set_server_ip(udpServerIPEditor.getText().toStdString());
			int port = udpServerPortEditor.getText().trim().getIntValue();
			if (port > 0 && port <= 65535)
			{
				std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->set_server_port(port);
			}
			isobus::CANStackLogger::info("Updated UDP CAN settings: " + udpServerIPEditor.getText().toStdString() + ":" + udpServerPortEditor.getText().toStdString());
		}

		if (nullptr != isobus::CANHardwareInterface::get_assigned_can_channel_frame_handler(0))
		{
			isobus::CANHardwareInterface::unassign_can_channel_frame_handler(0);
		}
		isobus::CANHardwareInterface::assign_can_channel_frame_handler(0, parentCANDrivers.at(hardwareInterfaceSelector.getSelectedId() - 1));
		isobus::CANStackLogger::info("Updated assigned CAN driver.");
#elif JUCE_LINUX
		std::static_pointer_cast<isobus::SocketCANInterface>(parentCANDrivers.at(0))->set_name(socketCANNameEditor.getText().toStdString());
		isobus::CANStackLogger::info("Updated socket CAN interface name to: " + socketCANNameEditor.getText().toStdString());

		// Update UDP settings
		if (parentCANDrivers.size() > 1)
		{
			std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->set_server_ip(udpServerIPEditor.getText().toStdString());
			int port = udpServerPortEditor.getText().trim().getIntValue();
			if (port > 0 && port <= 65535)
			{
				std::static_pointer_cast<isobus::UDPCANPlugin>(parentCANDrivers.at(1))->set_server_port(port);
			}
			isobus::CANStackLogger::info("Updated UDP CAN settings: " + udpServerIPEditor.getText().toStdString() + ":" + udpServerPortEditor.getText().toStdString());
		}
#endif
		parent.parentServer.save_settings();
	};
}

void ConfigureHardwareComponent::paint(Graphics &graphics)
{
	auto bounds = getLocalBounds();
	graphics.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	graphics.setColour(getLookAndFeel().findColour(Label::textColourId));
	graphics.setFont(16.0f);
#ifdef JUCE_WINDOWS
	graphics.drawFittedText("Select the CAN driver to use", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#elif JUCE_LINUX
	graphics.drawFittedText("Configure CAN Hardware Interface", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#elif defined(JUCE_MAC)
	graphics.drawFittedText("Configure CAN Hardware Interface", 10, 10, bounds.getWidth() - 20, 54, Justification::centredTop, 3);
#endif

	graphics.setFont(12.0f);

#ifdef JUCE_WINDOWS
	graphics.drawFittedText("Hardware Driver", hardwareInterfaceSelector.getBounds().getX(), hardwareInterfaceSelector.getBounds().getY() - 14, hardwareInterfaceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);

	if (3 == hardwareInterfaceSelector.getSelectedId())
	{
		graphics.drawFittedText("TouCAN Serial Number", touCANSerialEditor.getBounds().getX(), touCANSerialEditor.getBounds().getY() - 14, touCANSerialEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
	else if (5 == hardwareInterfaceSelector.getSelectedId())
	{
		graphics.drawFittedText("UDP Server IP Address", udpServerIPEditor.getBounds().getX(), udpServerIPEditor.getBounds().getY() - 14, udpServerIPEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
		graphics.drawFittedText("UDP Server Port", udpServerPortEditor.getBounds().getX(), udpServerPortEditor.getBounds().getY() - 14, udpServerPortEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
#elif defined(JUCE_MAC)
	graphics.drawFittedText("Hardware Driver", hardwareInterfaceSelector.getBounds().getX(), hardwareInterfaceSelector.getBounds().getY() - 14, hardwareInterfaceSelector.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	
	if (2 == hardwareInterfaceSelector.getSelectedId())
	{
		graphics.drawFittedText("UDP Server IP Address", udpServerIPEditor.getBounds().getX(), udpServerIPEditor.getBounds().getY() - 14, udpServerIPEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
		graphics.drawFittedText("UDP Server Port", udpServerPortEditor.getBounds().getX(), udpServerPortEditor.getBounds().getY() - 14, udpServerPortEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	}
#elif JUCE_LINUX
	graphics.drawFittedText("Socket CAN Interface Name", socketCANNameEditor.getBounds().getX(), socketCANNameEditor.getBounds().getY() - 14, socketCANNameEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	graphics.drawFittedText("UDP Server IP Address", udpServerIPEditor.getBounds().getX(), udpServerIPEditor.getBounds().getY() - 14, udpServerIPEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
	graphics.drawFittedText("UDP Server Port", udpServerPortEditor.getBounds().getX(), udpServerPortEditor.getBounds().getY() - 14, udpServerPortEditor.getBounds().getWidth(), 12, Justification::centredLeft, 1);
#endif
}

void ConfigureHardwareComponent::resized()
{
}
