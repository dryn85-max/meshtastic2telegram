# Meshtastic Firmware Modifications - Complete Changelog

This document provides a comprehensive list of all modifications made to the Meshtastic firmware for the Telegram Gateway project.

**Meshtastic Base Version:** 2.7.13.b1bcc86  
**Last Updated:** November 4, 2025  
**Project:** Meshtastic-Telegram Gateway v2.0

---

## Table of Contents
1. [Core Firmware Changes](#core-firmware-changes)
2. [Mesh Layer Changes](#mesh-layer-changes)
3. [Module Changes](#module-changes)
4. [Variant Configuration](#variant-configuration)
5. [Build System Changes](#build-system-changes)

---

## Core Firmware Changes

### 1. src/main.cpp

#### A. OTA Swap Bootloader Support

**Location:** Lines 48-49 (includes)
```cpp
#include "esp_ota_ops.h"
#include "esp_partition.h"
```
**Purpose:** Add ESP-IDF OTA partition management headers

---

**Location:** Lines 526-566 (in `setup()` function, after `OSThread::setup()`)
```cpp
#ifdef ARCH_ESP32
    // OTA SWAP BOOT LOGIC: Ensure User Bootloader (OTA_0) always runs first
    // This code runs on the Gateway firmware (OTA_1) to detect if it booted directly
    // (first boot after flash) and sets boot partition back to OTA_0 for next reboot
    
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    
    LOG_DEBUG("#### Running from: subtype %d @ 0x%X", running->subtype, running->address);
    LOG_DEBUG("#### Boot partition: subtype %d @ 0x%X", boot_partition->subtype, boot_partition->address);
    
    // Check if we're running from OTA_1 (Gateway) but boot partition is also OTA_1
    // This happens on first boot after flashing - we need to set boot to OTA_0
    if (running->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1 && 
        boot_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
        
        LOG_INFO("##############################################################################################################################################################");
        LOG_INFO("### FIRST BOOT AFTER FLASH DETECTED!");
        LOG_INFO("### Boot partition set to OTA_0 (User Bootloader)");
        LOG_INFO("### Next reboot will show splash screen!");
        LOG_INFO("   (Staying in Gateway mode for this session)");
        LOG_INFO("##############################################################################################################################################################");
        
        // Find OTA_0 partition (User Bootloader)
        const esp_partition_t *ota_0 = esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,
            ESP_PARTITION_SUBTYPE_APP_OTA_0,
            NULL
        );
        
        if (ota_0 != NULL) {
            // Set OTA_0 as the boot partition for next reboot
            esp_err_t err = esp_ota_set_boot_partition(ota_0);
            if (err == ESP_OK) {
                LOG_DEBUG("#### Successfully set boot partition to OTA_0 @ 0x%X", ota_0->address);
            } else {
                LOG_ERROR("#### Failed to set boot partition: 0x%X", err);
            }
        }
    }
#endif
```
**Purpose:** 
- Detect if Gateway firmware booted directly from flash (first boot)
- Set boot partition to OTA_0 (User Bootloader) for subsequent reboots
- Ensures splash screen always appears on reboot

**Implementation Notes:**
- Must be placed AFTER `OSThread::setup()` to ensure logging is initialized
- Only runs on ESP32 architecture
- Only triggers on first boot when both running and boot partitions are OTA_1

---

#### B. Device Role Logging

**Location:** Lines 822-823 (in `setup()` function, after `nodeDB` initialization)
```cpp
// Log device role (0=CLIENT, 2=ROUTER)
LOG_INFO("Device Role: %d", config.device.role);
```
**Purpose:** 
- Display active device role at startup for debugging
- Simplified version (single line) to reduce firmware size
- 0=CLIENT, 2=ROUTER, 1=CLIENT_MUTE, etc.

**Original Implementation (removed due to size):**
```cpp
// Full enum-to-string mapping (adds ~7KB to firmware)
const char *roleStr = "UNKNOWN";
switch (config.device.role) {
    case meshtastic_Config_DeviceConfig_Role_CLIENT: roleStr = "CLIENT"; break;
    case meshtastic_Config_DeviceConfig_Role_ROUTER: roleStr = "ROUTER"; break;
    // ... etc
}
LOG_INFO("📡 Device Role: %s", roleStr);
```

---

#### C. Motion Sensor Conditional Compilation

**Location:** Lines 131-134 (includes)
```cpp
#if !defined(ARCH_STM32WL) && !MESHTASTIC_EXCLUDE_I2C && HAS_ACCELEROMETER
#include "motion/AccelerometerThread.h"
AccelerometerThread *accelerometerThread = nullptr;
#endif
```
**Purpose:** Only include accelerometer code if explicitly enabled

---

**Location:** Lines 852-855 (in `setup()` function)
```cpp
#if !defined(ARCH_STM32WL) && !MESHTASTIC_EXCLUDE_I2C && HAS_ACCELEROMETER
    accelerometerThread = new AccelerometerThread();
#endif
```
**Purpose:** Only initialize accelerometer if hardware is present

**Rationale:** 
- Prevents compilation errors when motion sensors are excluded via `build_src_filter`
- Reduces firmware size by ~20KB when motion support is disabled

---

## Mesh Layer Changes

### 2. src/mesh/NodeDB.cpp

#### A. GATEWAY_MODE ROUTER Role Permission

**Location:** Lines 558-572 (in `installDefaultConfig()` function)
```cpp
#ifdef USERPREFS_CONFIG_DEVICE_ROLE
    // For GATEWAY_MODE builds, allow ROUTER role (needed for stationary infrastructure)
    #if defined(GATEWAY_MODE) && GATEWAY_MODE == 1
        config.device.role = USERPREFS_CONFIG_DEVICE_ROLE;
        LOG_INFO("Gateway Mode: Device role set to USERPREFS (ROUTER allowed)");
    #else
        // Restrict ROUTER*, LOST AND FOUND roles for security reasons on general devices
        if (IS_ONE_OF(USERPREFS_CONFIG_DEVICE_ROLE, meshtastic_Config_DeviceConfig_Role_ROUTER,
                      meshtastic_Config_DeviceConfig_Role_ROUTER_LATE, meshtastic_Config_DeviceConfig_Role_LOST_AND_FOUND)) {
            LOG_WARN("ROUTER roles are restricted, falling back to CLIENT role");
            config.device.role = meshtastic_Config_DeviceConfig_Role_CLIENT;
        } else {
            config.device.role = USERPREFS_CONFIG_DEVICE_ROLE;
        }
    #endif
#else
    config.device.role = meshtastic_Config_DeviceConfig_Role_CLIENT; // Default to client.
#endif
```

**Purpose:** 
- Bypass ROUTER role security restriction when `GATEWAY_MODE=1` is defined
- Allows setting ROUTER as default role for gateway deployments
- Maintains security restriction for non-gateway builds

**Background:**
- Meshtastic restricts ROUTER role by default for security (prevents mesh flooding)
- Gateway builds need ROUTER role for optimal stationary repeater performance
- Conditional compilation ensures security on mobile/portable devices

---

### 3. src/mesh/Router.cpp

#### A. Packet History Size Initialization

**Location:** Line 66 (in `Router` constructor)
```cpp
Router::Router() : concurrency::OSThread("Router"), PacketHistory(50), fromRadioQueue(MAX_RX_FROMRADIO)
```

**Original Code:**
```cpp
Router::Router() : concurrency::OSThread("Router"), fromRadioQueue(MAX_RX_FROMRADIO)
```

**Purpose:** 
- Explicitly initialize `PacketHistory` with size of 50 entries
- Prevents "Packet History - Invalid size -1, using default 100" warning
- Optimizes RAM usage (50 entries vs 100 default)

**Technical Details:**
- `Router` inherits from `PacketHistory`
- Without explicit initialization, uses default parameter of `-1`
- `-1` triggers validation check and fallback to `PACKETHISTORY_MAX`
- Explicit size avoids validation overhead and warning message

---

## Module Changes

### 4. src/modules/TelegramModule.cpp

#### A. WiFi AP Name Correction

**Location:** Line 453 (in config command response)
```cpp
response += "3. WiFi: MG-Config\n";
```

**Original Code:**
```cpp
response += "3. WiFi: Meshtastic-Config\n";
```

---

**Location:** Line 542 (in LoRa config instructions)
```cpp
response += "3. WiFi: MG-Config\n";
```

**Original Code:**
```cpp
response += "3. WiFi: Meshtastic-Config\n";
```

**Purpose:** 
- Correct WiFi AP SSID to match actual configuration ("MG-Config")
- Ensures user instructions are accurate
- Improves user experience by eliminating confusion

---

### 5. src/modules/TelegramCommandHandler.cpp

#### A. WiFi AP Name Correction

**Location:** Line 117 (in `/config` command response)
```cpp
response += "3. WiFi: MG-Config\n";
```

**Original Code:**
```cpp
response += "3. WiFi: Meshtastic-Config\n";
```

**Purpose:** Same as TelegramModule.cpp - consistency across all user-facing strings

---

## Variant Configuration

### 6. variants/esp32/diy/custom_sx1276_oled_telegram/platformio.ini

This is the complete build configuration for the Telegram Gateway variant. Key sections:

#### A. Environment Definition
```ini
[env:custom-sx1276-telegram-gateway]
extends = esp32_base
board = esp32doit-devkit-v1
board_check = true
```

#### B. Partition Configuration
```ini
board_build.partitions = variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv
board_upload.maximum_size = 3080192
```
**Purpose:** Use custom OTA swap partition table, allow 3MB firmware size

#### C. Source File Filtering
```ini
build_src_filter =
  ${esp32_base.build_src_filter}
  -<motion/>
  -<sensors/>
```
**Purpose:** Exclude motion and sensor source files to reduce firmware size

#### D. Library Dependencies
```ini
lib_deps =
  ${esp32_base.lib_deps}
  bblanchon/ArduinoJson@^6.21.3
  knolleary/PubSubClient@^2.8
  witnessmenow/UniversalTelegramBot@^1.3.0
```

#### E. Library Exclusions
```ini
lib_ignore =
  ${esp32_base.lib_ignore}
  Adafruit LIS3DH
  Adafruit BME280 Library
  # ... (18 sensor/motion libraries)
  NimBLE-Arduino
  ArduinoBLE
```
**Purpose:** Exclude unused libraries to reduce compilation time and firmware size

#### F. Display Configuration (Disabled)
```ini
-D NO_SCREEN=1
-D HAS_SCREEN=0
-D USE_SH1106=0
-D USE_SSD1306=0
```

#### G. Bluetooth Configuration (Disabled)
```ini
-D NO_BLE=1
-D CONFIG_BT_ENABLED=0
-D CONFIG_BLUEDROID_ENABLED=0
-D CONFIG_NIMBLE_ENABLED=0
-D MESHTASTIC_EXCLUDE_BLUETOOTH=1
```
**RAM Savings:** ~30KB

#### H. Gateway Mode Configuration
```ini
-D GATEWAY_MODE=1
-D TELEGRAM_ENABLED=1
-D TELEGRAM_USE_NVS=1
```
**Purpose:** Enable gateway-specific features and NVS configuration storage

#### I. Device Role Configuration
```ini
-D USERPREFS_CONFIG_DEVICE_ROLE=meshtastic_Config_DeviceConfig_Role_ROUTER
```
**Purpose:** Set ROUTER as default role for fresh installs

#### J. Module Exclusions
```ini
-D MESHTASTIC_EXCLUDE_AUDIO=1
-D MESHTASTIC_EXCLUDE_PAXCOUNTER=1
-D MESHTASTIC_EXCLUDE_STOREFORWARD=1
# ... (7 non-essential modules)
```
**RAM Savings:** ~15-20KB total

#### K. Buffer Optimizations
```ini
-D MAX_PACKETS=10
-D MAX_NUM_NODES=50
-D MESHTASTIC_MAX_CHANNELS=8
```
**Purpose:** Reduce buffer sizes for gateway use case

#### L. Packet History Configuration
```ini
-D PACKETHISTORY_MAX=50
```
**Purpose:** Explicitly set packet history size (must match `Router.cpp`)

#### M. Core Dump Configuration
```ini
-D CONFIG_ESP32_ENABLE_COREDUMP_TO_FLASH=0
-D CONFIG_ESP_COREDUMP_ENABLE_TO_FLASH=0
```
**Purpose:** Disable core dump feature to suppress bootloader warnings

---

### 7. variants/esp32/diy/custom_sx1276_oled_telegram/variant.h

#### A. Display Configuration (Conditional)
```cpp
#ifndef NO_SCREEN
#define I2C_SDA 21
#define I2C_SCL 22
#define USE_SSD1306 1
#define RESET_OLED 16
#endif
```
**Purpose:** Only define display pins if screen is enabled (disabled in platformio.ini)

#### B. Button and LED
```cpp
#define BUTTON_PIN 0   // BOOT button for config mode
#define LED_PIN 25     // Status LED
```

#### C. LoRa Radio Configuration
```cpp
#define USE_RF95 1          // SX1276/RFM95 radio
#define RF95_MAX_POWER 20   // Maximum TX power in dBm

#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
#define LORA_CS 18
#define LORA_DIO0 26
#define LORA_RESET 14
#define LORA_DIO1 33
#define LORA_DIO2 32
```

**Note:** `USE_RF95` is the Meshtastic convention for SX1276/RFM95 radios (not `USE_SX1276`)

---

### 8. variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv

```csv
# OTA Swap Partition Table for Meshtastic-Telegram Gateway
# 4MB Flash Layout: User Bootloader in OTA_0, Gateway in OTA_1
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x009000, 0x005000,
otadata,  data, ota,     0x00e000, 0x002000,
app0,     app,  ota_0,   0x010000, 0x0F0000,
app1,     app,  ota_1,   0x100000, 0x2F0000,
spiffs,   data, spiffs,  0x3F0000, 0x010000,
```

**Partition Breakdown:**
- **nvs (0x9000):** Non-Volatile Storage for WiFi/Telegram config - 20KB
- **otadata (0xE000):** OTA boot partition selector - 8KB
- **app0/OTA_0 (0x10000):** User Bootloader - 960KB
- **app1/OTA_1 (0x100000):** Gateway Firmware - 3MB (increased from 960KB)
- **spiffs (0x3F0000):** Configuration files - 64KB

**Key Change:** OTA_1 partition increased from 0x0F0000 (960KB) to 0x2F0000 (3MB) to accommodate larger firmware with Telegram support.

---

## Build System Changes

### Flash Script Compatibility

The `flash_complete_system.sh` script was updated to:
1. Build both User Bootloader and Gateway firmware
2. Generate partition table from CSV
3. Flash all components to correct addresses
4. Handle absolute paths for binary locations

**No changes to flash script documented here** - it's part of the bootloader system, not Meshtastic modifications.

---

## Summary of Benefits

### Features Enabled
- ✅ **OTA Swap Bootloader:** Professional boot experience with splash screen
- ✅ **ROUTER Mode:** Optimized for stationary gateway deployment
- ✅ **Telegram Integration:** Native bot support with NVS configuration
- ✅ **RAM Optimization:** 155KB free heap (vs 105KB with BLE/OLED)

### Issues Fixed
- ✅ **Packet History Warning:** Eliminated "Invalid size -1" error
- ✅ **WiFi AP Name:** Corrected to "MG-Config" everywhere
- ✅ **ROUTER Role Restriction:** Bypassed for gateway builds
- ✅ **Core Dump Warnings:** Suppressed cosmetic errors
- ✅ **Firmware Size:** Fits in 3MB partition (1.67MB actual)

### Performance Improvements
- ⚡ **Reduced Mesh Traffic:** ROUTER mode eliminates device telemetry broadcasts
- ⚡ **Faster Compilation:** Excluded 18+ unused libraries
- ⚡ **Lower RAM Usage:** Disabled BLE, OLED, and non-essential modules
- ⚡ **Optimized Buffers:** Reduced packet history and node limits for gateway use

---

## Testing Notes

### Verified Functionality
- ✅ User Bootloader boots first, displays splash, launches Gateway
- ✅ Gateway sets boot partition to OTA_0 on first run
- ✅ Device role correctly set to ROUTER (role=2)
- ✅ No packet history warnings in logs
- ✅ No device telemetry broadcasts at 60s intervals
- ✅ Telegram bot connects and responds to commands
- ✅ LoRa mesh packets are received and relayed
- ✅ WiFi reconnection after dropout works correctly

### Test Environment
- **Hardware:** ESP32-D0WDQ6 (revision v1.1), SX1276 LoRa radio
- **Flash:** 8MB (4MB partitions defined)
- **Network:** EU_868 region, LongFast preset
- **Firmware Size:** 1,678,272 bytes (1.60 MB)
- **Free Heap:** 155KB-158KB during operation

---

## Migration Notes

### Updating from CLIENT to ROUTER Mode

**Important:** Existing config files must be cleared for ROUTER mode to activate.

**Method 1: Factory Reset (Recommended)**
```bash
# Via serial monitor
factoryreset
```

**Method 2: Erase SPIFFS Partition**
```bash
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32 --port /dev/tty.usbserial-0001 --baud 460800 \
  erase_region 0x3F0000 0x10000
```

**Method 3: Full Flash Erase (Clean Slate)**
```bash
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py \
  --chip esp32 --port /dev/tty.usbserial-0001 \
  erase_flash
# Then reflash using flash_complete_system.sh
```

### Verifying ROUTER Mode is Active

Check serial log for:
```
INFO  | ??:??:?? 1 Device Role: 2
```

And verify NO telemetry at 60 seconds:
```
# This should NOT appear in ROUTER mode:
INFO  | ??:??:?? 60 [DeviceTelemetry] Send: air_util_tx=...
```

---

## Appendix: Define Reference

### Important Meshtastic Defines

| Define | Value | Purpose |
|--------|-------|---------|
| `GATEWAY_MODE` | `1` | Enables gateway-specific features |
| `TELEGRAM_ENABLED` | `1` | Enables Telegram module compilation |
| `NO_SCREEN` | `1` | Disables display support |
| `NO_BLE` | `1` | Disables Bluetooth support |
| `USERPREFS_CONFIG_DEVICE_ROLE` | `meshtastic_Config_DeviceConfig_Role_ROUTER` | Default device role |
| `PACKETHISTORY_MAX` | `50` | Packet history buffer size |
| `MAX_NUM_NODES` | `50` | Maximum nodes in database |
| `MAX_PACKETS` | `10` | Maximum packets in queue |

### Device Role Enum Values

| Role | Value | Description |
|------|-------|-------------|
| CLIENT | 0 | Default mobile device (sends telemetry) |
| CLIENT_MUTE | 1 | Mobile device (no telemetry) |
| ROUTER | 2 | Stationary repeater (no telemetry, full routing) |
| ROUTER_CLIENT | 3 | Hybrid mode |
| REPEATER | 4 | Basic repeater (deprecated) |
| TRACKER | 5 | GPS tracker (frequent position updates) |

---

**Document Version:** 1.0  
**Last Verified:** November 4, 2025  
**Maintainer:** Meshtastic-Telegram Gateway Project

