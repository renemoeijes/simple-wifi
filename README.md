# simple-wifi

This is part of a whole concept: 
- Simple-wifi  -> wifi ssid setup via phone.
- Simple-bt    -> Play bluetooth
- Simple-spot  -> Play spotify

Link to website:
https://renemoeijes.github.io/simple-wifi/

Install All Script:
https://raw.githubusercontent.com/renemoeijes/simple-wifi/main/Simple-Install.sh


**Simple WiFi Setup Portal for Raspberry Pi | Captive Portal | IoT WiFi Configuration**

simple-wifi is a minimal, high-performance **captive portal** designed specifically for **WiFi setup on Raspberry Pi** devices. 
Perfect for **IoT projects**, **headless Raspberry Pi setup**, and **embedded WiFi configuration**.

🚀 **Just connect to the Access Point and configure WiFi - no SSH, no keyboard needed!**

## Key Features

- **🔌 Plug & Play** - Automatically starts when no internet connection found
- **📱 Mobile-friendly** - Works with phones, tablets, and computers  
- **⚡ Lightweight** - Minimal resource usage, perfect for Pi Zero
- **🏗️ Production-ready** - Professional Debian packaging and systemd integration
- **🔧 Zero configuration** - Hardcoded settings, just install and use
- **🌐 Universal compatibility** - Works with all modern devices and browsers
- **🎯 IoT focused** - Designed for headless embedded WiFi setup

## Quick Start

1. **Install the package:**
   ```bash
      64 bit 
      sudo apt install /path/to/simple-wifi_<version>_arm64.deb

      Older 32 bit
      sudo apt install /path/to/simple-wifi_<version>_armhf.deb
   ```

2. **Boot your Pi** - simple-wifi starts automatically when no internet connection is found

3. **Connect to "simple-wifi Netwerk" WiFi** with your phone/computer - setup page appears automatically

4. **Enter your WiFi credentials** - portal stops automatically and Pi connects to your network

⚠️ **Important:** If you enter an incorrect WiFi password, you must reboot the device (unplug and reconnect power). Raspberry PI cannot check connection whilie in PA mode due to hardware / driver restrictions.

## Manual Control (Optional)

For testing or troubleshooting, you can manually control the portal:

- **Start manually:** `sudo StartPA`

*Note: Manual start only works if you have keyboard/monitor access or are already connected via SSH.*

## Building from Source

### Quick Build (for development)
```bash
make clean
make
sudo make install
```

### Complete Build from GitHub
```bash
git clone https://github.com/renemoeijes/simple-wifi.git
cd simple-wifi
make clean
make
sudo make install
```

### Building Debian Package
```bash
git clone https://github.com/renemoeijes/simple-wifi.git
cd simple-wifi
dpkg-buildpackage -b
```
This creates a `.deb` package in the parent directory that you can install with `sudo dpkg -i simple-wifi_*.deb`.

## About

simple-wifi is designed with usability as goal. To provide a simple solution for frustrating problems.
In this case for connecting a rpi to a wifi ssid. With no network connection, and no keyboard monitor this is always challenging.
So my solution is, when there is no internet, the device start it's own Access Point with SSID. When you connect your phone or computer to this network, the setup page start automatically, and you only need to select the network name (ssid) and enter the password for this network (ssid). 
So Simple !

**Author:** R. Moeijes  
**License:** GPL v2+  
**Project:** simple-wifi - SimpleSoft solutions

---
