# 🛰️ Meshtastic-Telegram Gateway

A custom ESP32 firmware that bridges Meshtastic mesh networks with Telegram, enabling remote monitoring and control of your mesh network via Telegram bot.

## ✨ Features

- **Full Meshtastic Mesh Node** – Complete routing, node database, packet history
- **ROUTER Mode Optimized** – Configured as stationary repeater (no device telemetry broadcasts)
- **Telegram Bot Integration** – Bidirectional message forwarding between mesh and Telegram
- **WiFi + SSL/TLS** – Secure connection to Telegram API
- **LoRa TX/RX** – Full mesh repeater functionality
- **Optimized RAM Usage** – ~155KB free RAM (BLE and OLED disabled)
- **OTA Swap Bootloader** – Custom Arduino user bootloader (OTA_0) shows splash, launches Gateway (OTA_1)
- **Interactive Commands** – `/nodes`, `/map`, `/status`, `/config` via Telegram
- **NVS Configuration** – WiFi, Telegram, and LoRa settings stored in Non-Volatile Storage

## 📊 System Requirements

- **Hardware:** ESP32 (no PSRAM) + SX1276/RFM95 LoRa module
- **RAM:** ~110KB used, ~155KB free (sufficient for SSL/TLS)
- **Flash:** 4MB (Gateway ~1.68MB + User Bootloader ~790KB)
- **Network:** 2.4GHz WiFi, Internet for Telegram

## 🏗️ Simplified Architecture

**Two firmware components:**
1. **`user_bootloader/`** (OTA_0) - Arduino-based bootloader with config portal built-in
2. **`Meshtastic_original/firmware/`** (OTA_1) - Full Gateway firmware

**That's it!** No boot selector, no separate config firmware - everything is consolidated.

## 🚀 Quick Start

### 1. Clone this repository

```bash
git clone <your-repo-url> meshtastic-telegram-gateway
cd meshtastic-telegram-gateway
```

### 2. Clone Meshtastic firmware

**Important:** The original Meshtastic firmware is not included in this repository. Clone it separately:

```bash
git clone https://github.com/meshtastic/firmware Meshtastic_original/firmware
cd Meshtastic_original/firmware
git checkout v2.7.13  # Or latest stable version
cd ../..
```

### 3. Install prerequisites

```bash
pip install platformio esptool
```

### 4. Apply modifications

Copy the modified Meshtastic files (see [`SETUP.md`](SETUP.md) for detailed instructions):

```bash
# Quick copy command
for file in modified_meshtastic_files/src/*.example; do
  cp "$file" "Meshtastic_original/firmware/src/$(basename ${file%.example})"
done
```

### 5. Build & Flash

```bash
./flash_complete_system.sh /dev/tty.usbserial-0001
```

This script:
- Builds the **User Bootloader** (`OTA_0 @ 0x10000`) - with splash + config AP
- Builds the **Gateway firmware** (`OTA_1 @ 0x100000`) - Meshtastic + Telegram
- Generates the OTA-swap partition table
- Flashes everything (bootloader, partitions, OTA_0, OTA_1)
- Opens serial monitor

### Manual flashing (if you prefer)

```bash
# Build user bootloader
cd user_bootloader
pio run -e user_bootloader

# Build gateway
cd ../Meshtastic_original/firmware
pio run -e custom-sx1276-telegram-gateway || true
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32 \
  elf2image --flash_mode dio --flash_freq 40m --flash_size 4MB \
  -o .pio/build/custom-sx1276-telegram-gateway/firmware.bin \
  .pio/build/custom-sx1276-telegram-gateway/firmware.elf

# Generate partition table
python3 ~/.platformio/packages/framework-arduinoespressif32/tools/gen_esp32part.py \
  Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv \
  partitions_ota_swap.bin

# Flash (adjust port)
esptool.py --chip esp32 --port /dev/tty.usbserial-0001 --baud 460800 \
  write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \
  0x1000   user_bootloader/.pio/build/user_bootloader/bootloader.bin \
  0x8000   partitions_ota_swap.bin \
  0x10000  user_bootloader/.pio/build/user_bootloader/firmware.bin \
  0x100000 Meshtastic_original/firmware/.pio/build/custom-sx1276-telegram-gateway/firmware.bin
```

### 6. Configure

After flashing, configure the gateway:

**Required settings:**
- WiFi SSID/Password
- Telegram Bot Token (from [@BotFather](https://t.me/botfather))
- Telegram Chat ID
- LoRa Region (e.g. `EU_868`, `US`)
- LoRa Modem Preset (e.g. `LONG_FAST`)

## 📱 Telegram Commands

- `/help` – Show available commands
- `/config` – View current configuration and setup instructions
- `/nodes` – List visible mesh nodes
- `/map` – Show GPS nodes on an interactive map
- `/status` – Gateway status and diagnostics
- Any text message – Broadcast to the mesh network

## 🏗️ Architecture

### OTA Swap Boot Flow ✅

```
Power On → ESP32 ROM bootloader → OTA_0 (User Bootloader)
         → Shows splash + diagnostics
         → Checks BOOT button (hold 3s for config mode)
         → Boots OTA_1 (Gateway) via esp_ota_set_boot_partition
         → Gateway runs (Meshtastic + Telegram)
         → Gateway sets boot partition back to OTA_0 for next reboot
```

**Partition Layout:**
- **OTA_0** (`0x10000`, 960KB): Arduino-based **User Bootloader** (splash screen, config AP mode)
- **OTA_1** (`0x100000`, 3MB): Full **Gateway firmware** (Meshtastic + Telegram + ROUTER mode)
- **OTA data** (`0xE000`, 8KB): Boot partition selector (managed automatically)
- **SPIFFS** (`0x3F0000`, 64KB): Configuration storage (WiFi, Telegram, LoRa settings)

### Gateway firmware includes

- Full Meshtastic mesh routing, node database, packet history
- LoRa TX/RX (full repeater)
- Position & telemetry modules
- Telegram bot with SSL/TLS
- WiFi STA mode

### Removed/disabled (to save RAM)

- **BLE/Bluetooth:** Disabled to save ~30KB RAM for SSL operations
- **OLED display:** Disabled to save ~15KB RAM for SSL operations
- Audio, store-forward, traceroute, canned message modules, etc.

**Note:** Initial configuration is done via User Bootloader's WiFi AP + Web Portal (no BLE needed)

## 🔧 Hardware Configuration

- **LoRa module:** SX1276/RFM95 (433/868/915 MHz)
- **Pins:** See `variant.h` in the custom variant directory
  - LoRa SPI: SCK=5, MISO=19, MOSI=27, CS=18
  - LoRa Control: IRQ=26, RESET=14
  - BOOT button (GPIO0) reserved for future AP/Web config trigger

## 🐛 Troubleshooting

### Common Issues

- **Gateway loops "LoRa tx disabled: Region unset"** → Configure LoRa region/modem in NVS
- **Telegram not responding** → Check Bot Token/Chat ID, WiFi, RAM (>100KB)
- **First boot loops twice** → Expected: User Bootloader sets boot partition, then runs Gateway
- **WiFi fails** → Confirm 2.4GHz network + correct credentials
- **"Device Role: 0" (CLIENT) instead of 2 (ROUTER)** → Perform factory reset to clear old config:
  ```
  # Via serial monitor:
  factoryreset
  
  # OR erase SPIFFS partition:
  python3 ~/.platformio/packages/tool-esptoolpy/esptool.py --chip esp32 \
    --port /dev/tty.usbserial-0001 erase_region 0x3F0000 0x10000
  ```
- **"Packet History - Invalid size -1" warning** → Update to latest firmware (fixed in Router.cpp)
- **Build error "program size too large"** → Ensure `board_upload.maximum_size = 3080192` is set in platformio.ini

### Verifying ROUTER Mode

Check serial output for:
```
INFO  | ??:??:?? 1 Device Role: 2
```

ROUTER mode indicators:
- ✅ No device telemetry broadcasts at 60-second intervals
- ✅ Full packet relaying and routing
- ✅ Lower mesh traffic from gateway itself

## 📚 Documentation

### Project Documentation
- [`docs/USER_BOOTLOADER_README.md`](docs/USER_BOOTLOADER_README.md) – Details of the OTA swap user bootloader
- [`docs/OTA_SWAP_README.md`](docs/OTA_SWAP_README.md) – Flash layout, boot flow, troubleshooting
- [`docs/AI_DEVELOPMENT_SPEC.md`](docs/AI_DEVELOPMENT_SPEC.md) – Complete technical specification for AI assistants
- [`docs/HOW_TO_REQUEST_FEATURES.md`](docs/HOW_TO_REQUEST_FEATURES.md) – Guide for requesting new features
- [`docs/hardware/pinout.md`](docs/hardware/pinout.md) – Hardware pinout and GPIO configuration

### Meshtastic Firmware Modifications
- `modified_meshtastic_files/` – **All modified Meshtastic firmware files with .example suffix**
- `modified_meshtastic_files/README.md` – Guide to applying modifications
- `modified_meshtastic_files/MODIFICATIONS_SUMMARY.md` – **Complete changelog with line numbers**

**⚠️ Important:** The Gateway firmware includes several modifications to the original Meshtastic codebase to enable:
- OTA swap bootloader support (boot partition management)
- ROUTER mode as default for gateway deployments
- Telegram module WiFi AP name corrections
- Packet history optimization (50 entries, no warnings)
- RAM optimizations (BLE/OLED/sensors disabled)

See `modified_meshtastic_files/MODIFICATIONS_SUMMARY.md` for detailed technical documentation of all changes.

## 🤝 Contributing

Help wanted on:
- User Bootloader AP + web config (save WiFi/Telegram/LoRa to NVS)
- Telegram command enhancements
- Additional diagnostics/telemetry
- Testing across board variants

## 📄 License

This project extends Meshtastic firmware. See [Meshtastic license](https://github.com/meshtastic/firmware) for details.

## 🙏 Acknowledgments

- [Meshtastic Project](https://meshtastic.org/)
- [UniversalTelegramBot](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)
- Arduino-ESP32 maintainers & contributors

---

**Status:** ✅ **Production Ready** - OTA swap bootloader architecture is fully functional and optimized for ROUTER mode gateway deployments.
