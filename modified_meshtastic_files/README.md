# Modified Meshtastic Firmware Files

This folder contains all files that have been modified from the original Meshtastic firmware for the Telegram Gateway project.

## 📁 Folder Structure

The structure mirrors the original Meshtastic firmware folder structure:
```
modified_meshtastic_files/
├── src/
│   ├── main.cpp.example                           # Main firmware entry point
│   ├── mesh/
│   │   ├── NodeDB.cpp.example                     # Node database & config management
│   │   └── Router.cpp.example                     # Packet routing & history
│   └── modules/
│       ├── TelegramModule.cpp.example             # Telegram bot integration
│       └── TelegramCommandHandler.cpp.example     # Telegram command handling
└── variants/esp32/diy/custom_sx1276_oled_telegram/
    ├── platformio.ini.example                     # Build configuration
    ├── variant.h.example                          # Hardware pin definitions
    └── partitions_ota_swap.csv.example            # Flash partition layout
```

## 🔧 How to Apply These Modifications

### Method 1: Copy Files Directly (Recommended)
1. **Remove the `.example` suffix** from each file
2. **Copy to the corresponding location** in your `Meshtastic_original/firmware/` directory
3. **Build the firmware** using `./flash_complete_system.sh`

```bash
# Example for main.cpp
cp modified_meshtastic_files/src/main.cpp.example \
   Meshtastic_original/firmware/src/main.cpp
```

### Method 2: Manual Application
Refer to `MODIFICATIONS_SUMMARY.md` for detailed change descriptions and apply them manually.

## 📝 File Descriptions

### Core Firmware Files

#### `src/main.cpp.example`
**Purpose:** Main firmware initialization and OTA swap boot logic

**Key Modifications:**
- **OTA Swap Detection** (lines 526-566): Detects if booting directly from OTA_1 and sets boot partition back to OTA_0
- **Device Role Logging** (line 823): Logs the active device role at startup for debugging
- **Motion Sensor Exclusion** (lines 131-134, 852-855): Conditionally excludes accelerometer code when disabled

#### `src/mesh/NodeDB.cpp.example`
**Purpose:** Node database management and default configuration

**Key Modifications:**
- **GATEWAY_MODE ROUTER Permission** (lines 558-572): Allows ROUTER role when `GATEWAY_MODE` is defined, bypassing security restriction

#### `src/mesh/Router.cpp.example`
**Purpose:** Packet routing and history management

**Key Modifications:**
- **PacketHistory Size** (line 66): Explicitly sets packet history size to 50 entries to prevent "Invalid size -1" warning

#### `src/modules/TelegramModule.cpp.example`
**Purpose:** Telegram bot integration and message handling

**Key Modifications:**
- **WiFi AP Name** (lines 453, 542): Corrected from "Meshtastic-Config" to "MG-Config"

#### `src/modules/TelegramCommandHandler.cpp.example`
**Purpose:** Telegram command parsing and response

**Key Modifications:**
- **WiFi AP Name** (line 117): Corrected from "Meshtastic-Config" to "MG-Config"

### Variant Configuration Files

#### `variants/esp32/diy/custom_sx1276_oled_telegram/platformio.ini.example`
**Purpose:** Complete build configuration for the Telegram Gateway

**Key Sections:**
- **Board Configuration:** ESP32 with 3MB OTA_1 partition
- **Display Disabled:** `NO_SCREEN=1`, all display libraries excluded
- **Bluetooth Disabled:** `NO_BLE=1`, all Bluetooth libraries excluded
- **Telegram Enabled:** `TELEGRAM_ENABLED=1`, `TELEGRAM_USE_NVS=1`
- **Gateway Mode:** `GATEWAY_MODE=1` (allows ROUTER role)
- **ROUTER Role Default:** `USERPREFS_CONFIG_DEVICE_ROLE=meshtastic_Config_DeviceConfig_Role_ROUTER`
- **Packet History:** `PACKETHISTORY_MAX=50`
- **Core Dump Disabled:** Suppresses core dump warnings
- **Buffer Optimizations:** Reduced buffer sizes for RAM optimization

#### `variants/esp32/diy/custom_sx1276_oled_telegram/variant.h.example`
**Purpose:** Hardware pin definitions

**Key Definitions:**
- **LoRa Radio:** SX1276/RFM95 on custom SPI pins
- **Display:** Conditionally defined (disabled in platformio.ini)
- **Button:** GPIO0 (BOOT button for config mode)
- **LED:** GPIO25

#### `variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv.example`
**Purpose:** Flash memory partition layout

**Key Partitions:**
- **OTA_0 (0x10000):** User Bootloader (960KB)
- **OTA_1 (0x100000):** Gateway Firmware (3MB)
- **SPIFFS (0x3F0000):** Configuration storage (64KB)

## 🎯 Summary of Changes

### Feature Additions
1. **OTA Swap Bootloader Support** - User Bootloader always runs first, then launches Gateway
2. **ROUTER Mode Default** - Gateway optimized for stationary repeater duty
3. **Telegram Integration** - Native Telegram bot with NVS configuration
4. **RAM Optimizations** - Bluetooth, Display, and non-essential modules disabled

### Bug Fixes
1. **Packet History Warning** - Fixed "Invalid size -1" error
2. **WiFi AP Name Consistency** - Corrected to "MG-Config" throughout
3. **ROUTER Role Restriction** - Bypassed for GATEWAY_MODE builds
4. **Core Dump Warnings** - Suppressed cosmetic errors

### Optimizations
1. **Firmware Size** - Fits in 3MB OTA_1 partition (1.67MB)
2. **RAM Usage** - ~155KB free heap (vs ~105KB with BLE/OLED)
3. **Mesh Efficiency** - ROUTER mode reduces unnecessary telemetry broadcasts

## 📚 Additional Documentation

For detailed implementation notes and step-by-step modifications, see:
- [`MODIFICATIONS_SUMMARY.md`](MODIFICATIONS_SUMMARY.md) - Complete changelog with line numbers
- [`../README.md`](../README.md) - Main project README
- [`../docs/OTA_SWAP_README.md`](../docs/OTA_SWAP_README.md) - OTA swap bootloader architecture
- [`../docs/USER_BOOTLOADER_README.md`](../docs/USER_BOOTLOADER_README.md) - User bootloader details
- [`../docs/hardware/pinout.md`](../docs/hardware/pinout.md) - Hardware configuration

## ⚠️ Important Notes

1. **These modifications are tested with Meshtastic firmware v2.7.13**
2. **Always backup your original Meshtastic firmware** before applying changes
3. **The `.example` suffix prevents accidental overwriting** - remove it when ready to use
4. **GATEWAY_MODE=1 must be defined** for ROUTER role to work
5. **Factory reset required** when changing from CLIENT to ROUTER role

## 🔄 Updating to Newer Meshtastic Versions

When updating to a newer Meshtastic version:
1. Check if the line numbers in `MODIFICATIONS_SUMMARY.md` still match
2. Review Meshtastic changelog for conflicting changes
3. Re-apply modifications carefully, line by line
4. Test thoroughly before deploying to production

## 🐛 Troubleshooting

### "Device Role: 0" (CLIENT) instead of 2 (ROUTER)
- **Solution:** Erase SPIFFS partition or perform factory reset to force default config creation

### "Packet History - Invalid size -1" warning
- **Solution:** Ensure `Router.cpp` explicitly initializes `PacketHistory(50)` in constructor

### Core dump errors at boot
- **Solution:** These are cosmetic ESP-IDF bootloader messages, can be suppressed with config flags

### Build fails with "program size too large"
- **Solution:** Ensure `board_upload.maximum_size = 3080192` is set in platformio.ini

## 📞 Support

For questions or issues:
1. Check the main [`README.md`](../README.md) for general setup instructions
2. Review [`docs/HOW_TO_REQUEST_FEATURES.md`](../docs/HOW_TO_REQUEST_FEATURES.md) for feature requests
3. Consult [Meshtastic documentation](https://meshtastic.org/) for firmware-specific questions

---

**Last Updated:** November 4, 2025  
**Meshtastic Version:** 2.7.13.b1bcc86  
**Project:** Meshtastic-Telegram Gateway v2.0

