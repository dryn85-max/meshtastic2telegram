# 🚀 Setup Guide - Meshtastic-Telegram Gateway

Complete setup instructions for building and flashing the Meshtastic-Telegram Gateway.

---

## 📋 **Prerequisites**

### Hardware Required
- ESP32 Dev Board (ESP32-WROOM, no PSRAM needed)
- SX1276/RFM95 LoRa Module (433/868/915 MHz)
- USB cable for programming

### Software Required
- Python 3.7+
- PlatformIO Core
- esptool.py
- Git

---

## 🔧 **Installation Steps**

### 1. Clone This Repository

```bash
git clone <your-repo-url> meshtastic-telegram-gateway
cd meshtastic-telegram-gateway
```

### 2. Clone Meshtastic Firmware

The original Meshtastic firmware is NOT included in this repository. Clone it separately:

```bash
git clone https://github.com/meshtastic/firmware Meshtastic_original/firmware
cd Meshtastic_original/firmware
git checkout v2.7.13  # Or latest stable version
cd ../..
```

### 3. Install PlatformIO

```bash
# Via pip
pip install platformio

# Or via package manager
brew install platformio  # macOS
```

### 4. Install esptool

```bash
pip install esptool
```

---

## 📝 **Apply Modifications**

The project includes modifications to Meshtastic firmware for Telegram integration. Apply them:

### Automatic Method (Recommended)

```bash
# Copy all modified files
cp modified_meshtastic_files/src/main.cpp.example \
   Meshtastic_original/firmware/src/main.cpp

cp modified_meshtastic_files/src/mesh/NodeDB.cpp.example \
   Meshtastic_original/firmware/src/mesh/NodeDB.cpp

cp modified_meshtastic_files/src/mesh/Router.cpp.example \
   Meshtastic_original/firmware/src/mesh/Router.cpp

# Copy variant files
cp modified_meshtastic_files/variants/esp32/diy/custom_sx1276_oled_telegram/platformio.ini.example \
   Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/platformio.ini

cp modified_meshtastic_files/variants/esp32/diy/custom_sx1276_oled_telegram/variant.h.example \
   Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/variant.h

cp modified_meshtastic_files/variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv.example \
   Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv
```

### Manual Method

See [`modified_meshtastic_files/README.md`](modified_meshtastic_files/README.md) for detailed modification instructions.

---

## 🔨 **Build & Flash**

### Option 1: Automated Script (Recommended)

```bash
./flash_complete_system.sh /dev/tty.usbserial-0001
```

Replace `/dev/tty.usbserial-0001` with your ESP32's serial port:
- macOS: `/dev/tty.usbserial-*` or `/dev/tty.SLAB_USBtoUART`
- Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
- Windows: `COM3`, `COM4`, etc.

### Option 2: Manual Build

```bash
# Build User Bootloader
cd user_bootloader
pio run -e user_bootloader
cd ..

# Build Gateway Firmware
cd Meshtastic_original/firmware
pio run -e custom-sx1276-telegram-gateway

# Create firmware.bin
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32 elf2image --flash_mode dio --flash_freq 40m --flash_size 4MB \
  -o .pio/build/custom-sx1276-telegram-gateway/firmware.bin \
  .pio/build/custom-sx1276-telegram-gateway/firmware.elf
cd ../..

# Generate partition table
python3 ~/.platformio/packages/framework-arduinoespressif32/tools/gen_esp32part.py \
  Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv \
  .pio/partitions_ota_swap.bin

# Flash everything
esptool.py --chip esp32 --port /dev/tty.usbserial-0001 --baud 460800 \
  write_flash -z \
  0x1000   user_bootloader/.pio/build/user_bootloader/bootloader.bin \
  0x8000   .pio/partitions_ota_swap.bin \
  0x10000  user_bootloader/.pio/build/user_bootloader/firmware.bin \
  0x100000 Meshtastic_original/firmware/.pio/build/custom-sx1276-telegram-gateway/firmware.bin
```

---

## ⚙️ **Configuration**

### First Boot Configuration

1. **Power on the ESP32**
2. **Hold BOOT button** for 3 seconds during startup
3. **Connect to WiFi**: `MeshGateway-Setup` (password: `meshtastic`)
4. **Open browser**: `http://192.168.4.1`
5. **Configure**:
   - WiFi SSID & Password
   - Telegram Bot Token (from [@BotFather](https://t.me/botfather))
   - Telegram Chat ID
   - LoRa Region (e.g., EU_868, US_915)
   - LoRa Modem Preset (e.g., LONG_FAST)
6. **Click Save** - Device will reboot and connect

---

## 📱 **Create Telegram Bot**

1. **Open Telegram** and search for [@BotFather](https://t.me/botfather)
2. **Send** `/newbot`
3. **Follow prompts** to create bot
4. **Copy Bot Token** (e.g., `123456:ABC-DEF1234ghIkl-zyx57W2v1u123ew11`)
5. **Get Chat ID**:
   - Send a message to your bot
   - Visit: `https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates`
   - Copy `chat.id` from response

---

## 🔍 **Verification**

### Check Serial Monitor

```bash
python3 monitor.py /dev/tty.usbserial-0001 115200
```

Expected output:
```
╔════════════════════════════════════════════════════════╗
║     🛰️  Meshtastic-Telegram Gateway v2.0             ║
╚════════════════════════════════════════════════════════╝

✅ Telegram Bot Active
Device Role: 2 (ROUTER)
Free RAM: ~173KB
```

### Test Telegram Commands

Send to your bot:
- `/help` - List commands
- `/status` - Gateway status
- `/config` - View configuration
- `/nodes` - List mesh nodes

---

## 📚 **Documentation**

- [`README.md`](README.md) - Project overview
- [`docs/USER_BOOTLOADER_README.md`](docs/USER_BOOTLOADER_README.md) - Bootloader details
- [`docs/OTA_SWAP_README.md`](docs/OTA_SWAP_README.md) - Architecture documentation
- [`docs/hardware/pinout.md`](docs/hardware/pinout.md) - Hardware configuration
- [`modified_meshtastic_files/MODIFICATIONS_SUMMARY.md`](modified_meshtastic_files/MODIFICATIONS_SUMMARY.md) - Technical changes

---

## 🐛 **Troubleshooting**

### Build Errors

**"Meshtastic_original/firmware not found"**
- Clone Meshtastic firmware: `git clone https://github.com/meshtastic/firmware Meshtastic_original/firmware`

**"platformio: command not found"**
- Install PlatformIO: `pip install platformio`

### Flash Errors

**"Failed to connect to ESP32"**
- Hold BOOT button while connecting
- Check USB cable supports data (not just power)
- Try different baud rate: `--baud 115200`

**"Device reboots constantly"**
- Perform factory reset:
  ```bash
  esptool.py --chip esp32 --port /dev/tty.usbserial-0001 erase_flash
  ./flash_complete_system.sh /dev/tty.usbserial-0001
  ```

### Runtime Errors

**"Telegram not responding"**
- Check WiFi configuration
- Verify Bot Token and Chat ID
- Ensure internet connectivity

**"No mesh nodes visible"**
- You need at least 2 Meshtastic nodes
- Check LoRa region matches your hardware
- Verify antenna is connected

---

## 🆘 **Support**

For issues and questions:
1. Check [`docs/`](docs/) folder for detailed documentation
2. Review [`modified_meshtastic_files/MODIFICATIONS_SUMMARY.md`](modified_meshtastic_files/MODIFICATIONS_SUMMARY.md)
3. Open an issue on GitHub

---

## 📄 **License**

This project extends Meshtastic firmware (GPL-3.0). See [LICENSE](LICENSE) for details.

