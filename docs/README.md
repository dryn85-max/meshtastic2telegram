# 📚 Documentation Index

Complete documentation for the Meshtastic-Telegram Gateway project.

---

## 🚀 Getting Started

**New to this project?** Start here:

1. **[Main README](../README.md)** - Project overview and quick start
2. **[SETUP.md](../SETUP.md)** - Complete installation guide
3. **[User Bootloader Guide](USER_BOOTLOADER_README.md)** - Configuration portal details

---

## 📖 Core Documentation

### Architecture & Design

- **[OTA Swap Architecture](OTA_SWAP_README.md)**  
  Detailed explanation of the dual-partition boot system (User Bootloader + Gateway firmware)

- **[User Bootloader](USER_BOOTLOADER_README.md)**  
  WiFi configuration portal, AP mode, and credential management

- **[Hardware Pinout](hardware/pinout.md)**  
  GPIO connections for ESP32 + SX1276/RFM95 LoRa module

### Firmware Modifications

- **[Meshtastic Modifications](MESHTASTIC_MODIFICATIONS.md)**  
  Technical details of all Meshtastic firmware changes for Telegram integration

- **[Changelog](CHANGELOG.md)**  
  Version history and migration guides

---

## 🔧 Technical Reference

### Build & Flash

- **[Setup Guide](../SETUP.md)** - Complete build and flash instructions
- **[Flash Script](../flash_complete_system.sh)** - Automated build and flash

### Configuration

- **WiFi Setup** - See [User Bootloader Guide](USER_BOOTLOADER_README.md#configuration-portal)
- **Telegram Bot** - See [Setup Guide](../SETUP.md#create-telegram-bot)
- **LoRa Settings** - Configured via User Bootloader web portal

---

## 📁 Project Structure

```
meshtastic-telegram-gateway/
├── README.md                      # Main project overview
├── SETUP.md                       # Complete setup guide
├── LICENSE                        # GPL-3.0 License
├── flash_complete_system.sh       # Build and flash script
├── monitor.py                     # Serial monitor utility
│
├── docs/                          # Documentation (you are here)
│   ├── README.md                  # This file
│   ├── OTA_SWAP_README.md         # Boot architecture
│   ├── USER_BOOTLOADER_README.md  # Bootloader guide
│   ├── MESHTASTIC_MODIFICATIONS.md # Technical changes
│   ├── CHANGELOG.md               # Version history
│   └── hardware/
│       └── pinout.md              # Hardware connections
│
├── user_bootloader/               # User Bootloader source
│   ├── platformio.ini
│   └── src/main.cpp
│
├── modified_meshtastic_files/     # Firmware modifications
│   ├── README.md                  # How to apply
│   ├── src/*.example              # Modified source files
│   └── variants/*.example         # Hardware config
│
├── webapp/                        # Telegram configuration UI
│   ├── index.html
│   ├── app.js
│   └── styles.css
│
└── Meshtastic_original/           # Original firmware (clone separately)
    └── firmware/                  # git clone from meshtastic/firmware
```

---

## 🆘 Troubleshooting

### Build Issues

**Problem:** Cannot find Meshtastic firmware  
**Solution:** Clone it separately:
```bash
git clone https://github.com/meshtastic/firmware Meshtastic_original/firmware
```

**Problem:** PlatformIO errors  
**Solution:** 
```bash
pip install --upgrade platformio
pio update
```

### Flash Issues

**Problem:** Cannot connect to ESP32  
**Solution:** Hold BOOT button while connecting, try different baud rate

**Problem:** Device reboots constantly  
**Solution:** Erase flash and reflash:
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash
./flash_complete_system.sh /dev/ttyUSB0
```

### Runtime Issues

**Problem:** Cannot enter configuration mode  
**Solution:** Hold BOOT button for 3+ seconds during startup

**Problem:** WiFi not connecting  
**Solution:** Re-enter configuration mode, verify credentials, check WiFi signal

**Problem:** Telegram not responding  
**Solution:** Verify bot token, check internet connectivity, review serial output

---

## 📄 License

This project extends Meshtastic firmware. See [LICENSE](../LICENSE) for details (GPL-3.0).

---

## 🔗 External Resources

- [Meshtastic Firmware](https://github.com/meshtastic/firmware) - Upstream firmware
- [Meshtastic Documentation](https://meshtastic.org/docs/) - Official Meshtastic docs
- [Telegram Bot API](https://core.telegram.org/bots) - Telegram bot documentation
- [PlatformIO](https://platformio.org/) - Build system documentation

---

**Last Updated:** November 2025
