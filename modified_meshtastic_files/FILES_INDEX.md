# Modified Files Index

Quick reference for all modified Meshtastic firmware files.

## Files Modified

### Core Firmware (3 files)

1. **`src/main.cpp.example`** (1668 lines)
   - **Changes:** Lines 48-49, 526-566, 822-823, 131-134, 852-855
   - **Purpose:** OTA swap logic, device role logging, motion sensor exclusion
   - **Impact:** Critical for bootloader integration

2. **`src/mesh/NodeDB.cpp.example`** (~1200 lines estimated)
   - **Changes:** Lines 558-572
   - **Purpose:** Allow ROUTER role when GATEWAY_MODE=1
   - **Impact:** Required for ROUTER mode default

3. **`src/mesh/Router.cpp.example`** (804 lines)
   - **Changes:** Line 66
   - **Purpose:** Explicit PacketHistory(50) initialization
   - **Impact:** Fixes "Invalid size -1" warning

### Telegram Modules (2 files)

4. **`src/modules/TelegramModule.cpp.example`** (995 lines)
   - **Changes:** Lines 453, 542
   - **Purpose:** WiFi AP name correction (MG-Config)
   - **Impact:** User-facing text accuracy

5. **`src/modules/TelegramCommandHandler.cpp.example`** (~300 lines estimated)
   - **Changes:** Line 117
   - **Purpose:** WiFi AP name correction (MG-Config)
   - **Impact:** User-facing text accuracy

### Variant Configuration (3 files)

6. **`variants/esp32/diy/custom_sx1276_oled_telegram/platformio.ini.example`** (147 lines)
   - **Changes:** Entire file (custom configuration)
   - **Purpose:** Complete build configuration
   - **Impact:** Defines all build flags, libraries, optimizations
   - **Key Sections:**
     - Board and partition configuration
     - Display disabled (NO_SCREEN=1)
     - Bluetooth disabled (NO_BLE=1)
     - Telegram enabled (TELEGRAM_ENABLED=1)
     - Gateway mode (GATEWAY_MODE=1)
     - ROUTER mode default (USERPREFS_CONFIG_DEVICE_ROLE)
     - Packet history size (PACKETHISTORY_MAX=50)
     - Core dump disabled
     - Buffer optimizations

7. **`variants/esp32/diy/custom_sx1276_oled_telegram/variant.h.example`** (59 lines)
   - **Changes:** Lines 21-59 (entire file)
   - **Purpose:** Hardware pin definitions
   - **Impact:** Defines LoRa SPI pins, button, LED
   - **Key Definitions:**
     - Display pins (conditional, disabled in platformio.ini)
     - BUTTON_PIN = 0 (BOOT button)
     - LED_PIN = 25
     - LoRa radio pins (SPI + control)

8. **`variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv.example`** (10 lines)
   - **Changes:** Line 7 (OTA_1 size increased)
   - **Purpose:** Flash partition layout
   - **Impact:** Allows 3MB firmware vs 960KB
   - **Critical Change:** `app1` size changed from 0x0F0000 to 0x2F0000

## File Relationships

```
platformio.ini ──────────> Defines build flags
      │                     └─> GATEWAY_MODE=1
      │                     └─> ROUTER mode default
      │                     └─> PACKETHISTORY_MAX=50
      │
      ├──> variant.h ──────> Hardware pin definitions
      │
      └──> partitions_ota_swap.csv ──> Flash layout (3MB OTA_1)

main.cpp ────────────────> Uses OTA APIs for boot management
                            Logs device role
                            Conditionally includes motion sensors

NodeDB.cpp ──────────────> Reads GATEWAY_MODE flag
                            Allows ROUTER role if gateway build

Router.cpp ──────────────> Reads PACKETHISTORY_MAX define
                            Initializes with explicit size

TelegramModule.cpp ──────> User-facing strings (WiFi AP name)
TelegramCommandHandler.cpp ──> User-facing strings (WiFi AP name)
```

## Application Order

### For New Installations

1. **First**, apply variant configuration files:
   ```bash
   cp modified_meshtastic_files/variants/esp32/diy/custom_sx1276_oled_telegram/platformio.ini.example \
      Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/platformio.ini
   
   cp modified_meshtastic_files/variants/esp32/diy/custom_sx1276_oled_telegram/variant.h.example \
      Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/variant.h
   
   cp modified_meshtastic_files/variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv.example \
      Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv
   ```

2. **Second**, apply core firmware modifications:
   ```bash
   cp modified_meshtastic_files/src/main.cpp.example \
      Meshtastic_original/firmware/src/main.cpp
   
   cp modified_meshtastic_files/src/mesh/NodeDB.cpp.example \
      Meshtastic_original/firmware/src/mesh/NodeDB.cpp
   
   cp modified_meshtastic_files/src/mesh/Router.cpp.example \
      Meshtastic_original/firmware/src/mesh/Router.cpp
   ```

3. **Third**, apply module modifications:
   ```bash
   cp modified_meshtastic_files/src/modules/TelegramModule.cpp.example \
      Meshtastic_original/firmware/src/modules/TelegramModule.cpp
   
   cp modified_meshtastic_files/src/modules/TelegramCommandHandler.cpp.example \
      Meshtastic_original/firmware/src/modules/TelegramCommandHandler.cpp
   ```

4. **Finally**, build and flash:
   ```bash
   ./flash_complete_system.sh /dev/tty.usbserial-0001
   ```

### For Updates to Existing Installation

If you already have a working gateway and want to update specific features:

**To enable ROUTER mode:**
- Update `NodeDB.cpp` and `platformio.ini`
- Erase SPIFFS partition to force new default config

**To fix packet history warning:**
- Update `Router.cpp` only

**To fix WiFi AP name:**
- Update `TelegramModule.cpp` and `TelegramCommandHandler.cpp`

**To enable OTA swap bootloader:**
- Update `main.cpp` and rebuild

## File Sizes

| File | Lines | Size (approx) |
|------|-------|---------------|
| main.cpp | 1668 | ~65KB |
| NodeDB.cpp | ~1200 | ~50KB |
| Router.cpp | 804 | ~32KB |
| TelegramModule.cpp | 995 | ~40KB |
| TelegramCommandHandler.cpp | ~300 | ~12KB |
| platformio.ini | 147 | ~6KB |
| variant.h | 59 | ~2KB |
| partitions_ota_swap.csv | 10 | ~0.5KB |

**Total modified code:** ~208KB across 8 files

## Testing Checklist

After applying modifications, verify:

- [ ] **Build succeeds** without errors
- [ ] **Firmware size** is under 3MB (check build output)
- [ ] **User Bootloader boots first** (see splash screen)
- [ ] **Gateway firmware launches** from User Bootloader
- [ ] **Device Role: 2** appears in serial log
- [ ] **No packet history warning** in serial log
- [ ] **No telemetry at 60s** (ROUTER mode behavior)
- [ ] **Telegram bot connects** successfully
- [ ] **LoRa packets** are received and relayed
- [ ] **WiFi AP name** is "MG-Config" in config instructions

## Backup Recommendation

Before applying modifications, backup these original files:

```bash
# Create backup directory
mkdir -p ~/meshtastic_backups/$(date +%Y%m%d)

# Backup files
cp Meshtastic_original/firmware/src/main.cpp ~/meshtastic_backups/$(date +%Y%m%d)/
cp Meshtastic_original/firmware/src/mesh/NodeDB.cpp ~/meshtastic_backups/$(date +%Y%m%d)/
cp Meshtastic_original/firmware/src/mesh/Router.cpp ~/meshtastic_backups/$(date +%Y%m%d)/
# ... etc
```

## Version Compatibility

These modifications are tested with:
- **Meshtastic Firmware:** v2.7.13.b1bcc86
- **PlatformIO Core:** 6.1.x
- **Arduino-ESP32:** 2.0.x (via PlatformIO)

**When upgrading Meshtastic:**
1. Compare line numbers in MODIFICATIONS_SUMMARY.md
2. Review Meshtastic changelog for conflicts
3. Re-apply modifications carefully
4. Test thoroughly before production use

## Quick Reference: What Each File Does

| File | Primary Purpose | Required For |
|------|----------------|--------------|
| `main.cpp` | Boot logic & initialization | OTA swap, role logging |
| `NodeDB.cpp` | Config management | ROUTER mode default |
| `Router.cpp` | Packet routing | Packet history fix |
| `TelegramModule.cpp` | Bot integration | WiFi AP name fix |
| `TelegramCommandHandler.cpp` | Command parsing | WiFi AP name fix |
| `platformio.ini` | Build configuration | All optimizations |
| `variant.h` | Pin definitions | Hardware compatibility |
| `partitions_ota_swap.csv` | Flash layout | 3MB firmware support |

---

**Last Updated:** November 4, 2025  
**Total Files Modified:** 8  
**Total Lines Changed:** ~200 (excluding platformio.ini which is all new)  
**Firmware Size:** 1,678,272 bytes (1.60 MB compiled)

