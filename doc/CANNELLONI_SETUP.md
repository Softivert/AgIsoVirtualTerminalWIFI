# Cannelloni Setup Guide

This guide explains how to set up cannelloni on a Raspberry Pi to bridge CAN traffic to AgIsoVirtualTerminal running on a Windows tablet via UDP/TCP.

## Architecture

```
[CAN Equipment] ←→ [Raspberry Pi + cannelloni] ←→ [UDP/TCP Network] ←→ [Windows Tablet + AgIsoVirtualTerminal]
```

## Prerequisites

### On Raspberry Pi
- Raspberry Pi with CAN interface (e.g., PiCAN, MCP2515-based HAT)
- Raspbian OS or similar Linux distribution
- Network connectivity (WiFi or Ethernet)

### On Windows Tablet
- Windows 10 or later
- AgIsoVirtualTerminal installed
- Network connectivity (WiFi or Ethernet) to the same network as Raspberry Pi

## Installation on Raspberry Pi

### 1. Install Dependencies

```bash
sudo apt-get update
sudo apt-get install -y git cmake build-essential
sudo apt-get install -y can-utils
```

### 2. Configure CAN Interface

Edit `/boot/config.txt` (or `/boot/firmware/config.txt` on newer systems) and add:

```
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=25
dtoverlay=spi-bcm2835
```

Note: Adjust the oscillator frequency (16000000 or 8000000) and interrupt pin according to your CAN HAT specifications.

Reboot the Raspberry Pi:
```bash
sudo reboot
```

### 3. Verify CAN Interface

After reboot, verify that the CAN interface is available:

```bash
ip link show can0
```

### 4. Set Up CAN Interface

Create a script to initialize the CAN interface at boot. Create `/usr/local/bin/setup-can.sh`:

```bash
#!/bin/bash
sudo ip link set can0 type can bitrate 250000
sudo ip link set can0 up
```

Make it executable:
```bash
sudo chmod +x /usr/local/bin/setup-can.sh
```

Add to `/etc/rc.local` before the `exit 0` line:
```
/usr/local/bin/setup-can.sh
```

Or create a systemd service for it.

### 5. Install Cannelloni

```bash
# Clone cannelloni repository
cd ~
git clone https://github.com/mguentner/cannelloni.git
cd cannelloni

# Build cannelloni
mkdir build
cd build
cmake ..
make
sudo make install
```

### 6. Configure Cannelloni Service

Create a systemd service file `/etc/systemd/system/cannelloni.service`:

```ini
[Unit]
Description=Cannelloni CAN over UDP/TCP
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/cannelloni -I can0 -R <TABLET_IP> -r 20000 -l 20000
Restart=on-failure
RestartSec=5
User=root

[Install]
WantedBy=multi-user.target
```

Replace `<TABLET_IP>` with the IP address of your Windows tablet.

Enable and start the service:

```bash
sudo systemctl daemon-reload
sudo systemctl enable cannelloni
sudo systemctl start cannelloni
```

Check the status:
```bash
sudo systemctl status cannelloni
```

## Configuration on Windows Tablet (AgIsoVirtualTerminal)

### 1. Find Your Tablet's IP Address

Open Command Prompt and run:
```cmd
ipconfig
```

Note the IPv4 address of your network adapter (e.g., `192.168.1.100`).

### 2. Configure AgIsoVirtualTerminal

1. Launch AgIsoVirtualTerminal
2. Go to the Hardware Configuration menu
3. Select **UDP CAN** as the hardware driver
4. Enter the following settings:
   - **UDP Server IP**: Enter the IP address of your Raspberry Pi (e.g., `192.168.1.50`)
   - **UDP Server Port**: Enter `20000` (or the port configured in cannelloni)
5. Click **OK** to save the settings

### 3. Start Communication

The UDP CAN plugin will automatically connect to the cannelloni server on the Raspberry Pi and begin forwarding CAN frames.

## Troubleshooting

### Raspberry Pi

**Check CAN interface status:**
```bash
ip -details -statistics link show can0
```

**Monitor CAN traffic:**
```bash
candump can0
```

**Check cannelloni logs:**
```bash
sudo journalctl -u cannelloni -f
```

**Test cannelloni manually:**
```bash
sudo cannelloni -I can0 -R <TABLET_IP> -r 20000 -l 20000 -v
```

### Windows Tablet

**Check network connectivity:**
```cmd
ping <RASPBERRY_PI_IP>
```

**Verify firewall settings:**
- Make sure Windows Firewall allows UDP traffic on port 20000
- Add an inbound rule if needed

### Common Issues

1. **No CAN traffic visible:**
   - Verify CAN bus is properly connected and terminated
   - Check CAN bitrate matches your equipment (250kbps is common for ISOBUS)
   - Use `candump can0` on the Raspberry Pi to verify CAN frames are being received

2. **UDP connection fails:**
   - Verify both devices are on the same network
   - Check firewall settings on both devices
   - Verify IP addresses are correct

3. **Frames being dropped:**
   - Check network quality and latency
   - Consider using wired Ethernet instead of WiFi for more stable connection
   - Monitor system resources on both devices

## Advanced Configuration

### Using Different Ports

To use different ports, update both:

**On Raspberry Pi (in cannelloni.service):**
```
-l 20000    # Local port
-r 20000    # Remote port
```

**On Windows Tablet:**
Update the UDP Server Port in AgIsoVirtualTerminal configuration.

### TCP Mode (More Reliable)

For more reliable communication, use TCP mode:

**On Raspberry Pi:**
```bash
sudo cannelloni -I can0 -R <TABLET_IP> -r 20000 -l 20000 -t
```

Add the `-t` flag to use TCP instead of UDP.

Note: TCP mode is not yet supported in the current version of AgIsoVirtualTerminal's UDP CAN plugin, but may be added in future versions.

### Multiple CAN Interfaces

If you have multiple CAN interfaces, you can run multiple cannelloni instances on different ports.

## Performance Considerations

- **Network latency:** WiFi can introduce variable latency. Use wired Ethernet for critical applications.
- **Frame buffering:** Cannelloni includes buffering to handle burst traffic.
- **System load:** Monitor CPU usage on Raspberry Pi, especially with high CAN bus loads.

## References

- [Cannelloni GitHub Repository](https://github.com/mguentner/cannelloni)
- [SocketCAN Documentation](https://www.kernel.org/doc/html/latest/networking/can.html)
- [ISOBUS Standard (ISO 11783)](https://www.iso.org/standard/57556.html)
- [AgIsoVirtualTerminal GitHub](https://github.com/Open-Agriculture/AgIsoVirtualTerminal)
