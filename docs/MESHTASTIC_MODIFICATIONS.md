# Meshtastic Firmware Modifications

**⚠️ CRITICAL: This document describes all modifications made to the original Meshtastic firmware to enable the OTA Swap User Bootloader architecture.**

**Date:** 2025-11-04  
**Meshtastic Version Base:** 2.7.14  
**Modified for:** Telegram Gateway with User Bootloader

---

## 📋 Summary

To enable the OTA Swap bootloader (User Bootloader in OTA_0 → Gateway in OTA_1), we made:
1. **Created** a new custom variant (non-destructive)
2. **Modified** `src/main.cpp` to handle boot partition switching (⚠️ modifies original)

---

## 1️⃣ New Custom Variant (Safe Addition)

### Files Created:
```
Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/
├── platformio.ini
├── variant.h
└── partitions_ota_swap.csv
```

These files define a new board variant alongside existing Meshtastic boards. **Does not modify any original Meshtastic code.**

### Purpose:
- Define hardware configuration (SX1276/RFM95 LoRa pins)
- Disable RAM-heavy features (Bluetooth, OLED) for SSL/TLS headroom
- Specify OTA swap partition table

---

## 2️⃣ Critical Modification: `src/main.cpp`

### ⚠️ **File Modified:**
```
Meshtastic_original/firmware/src/main.cpp
```

### **Change #1: Add ESP32 OTA Headers**

**Location:** After line 47 (`#include "freertosinc.h"`)

**Add these 2 lines:**
```cpp
#include "esp_ota_ops.h"
#include "esp_partition.h"
```

**Complete context (lines 46-57):**
```cpp
#ifdef ARCH_ESP32
#include "freertosinc.h"
#include "esp_ota_ops.h"           // ← ADD THIS
#include "esp_partition.h"         // ← ADD THIS
#if !MESHTASTIC_EXCLUDE_WEBSERVER
#include "mesh/http/WebServer.h"
#endif
#if !MESHTASTIC_EXCLUDE_BLUETOOTH
#include "nimble/NimbleBluetooth.h"
NimbleBluetooth *nimbleBluetooth = nullptr;
#endif
#endif
```

---

### **Change #2: Add OTA Swap Detection Logic**

**Location:** In `void setup()`, immediately AFTER `OSThread::setup();` (line ~524)

**Why after OSThread::setup():**  
Serial/logging is initialized by `OSThread::setup()`. If we add the code before that, LOG_INFO/LOG_DEBUG messages won't print!

**Insert this block:**
```cpp
#ifdef ARCH_ESP32
    // OTA Swap Bootloader: If we're running from OTA_1 but boot partition is also OTA_1,
    // set boot partition to OTA_0 (User Bootloader) for next reboot.
    // This ensures the splash screen shows on all subsequent boots.
    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    
    if (running_partition && boot_partition) {
        LOG_DEBUG("🔍 Running from: subtype %d @ 0x%x", running_partition->subtype, running_partition->address);
        LOG_DEBUG("🔍 Boot partition: subtype %d @ 0x%x", boot_partition->subtype, boot_partition->address);
        
        // If we're running from OTA_1 AND boot partition is OTA_1, switch boot to OTA_0
        if (running_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1 &&
            boot_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
            
            const esp_partition_t *ota0 = esp_partition_find_first(
                ESP_PARTITION_TYPE_APP,
                ESP_PARTITION_SUBTYPE_APP_OTA_0,
                NULL
            );
            
            if (ota0) {
                esp_err_t err = esp_ota_set_boot_partition(ota0);
                if (err == ESP_OK) {
                    LOG_INFO("════════════════════════════════════════════════════════");
                    LOG_INFO("✅ FIRST BOOT AFTER FLASH DETECTED!");
                    LOG_INFO("✅ Boot partition set to OTA_0 (User Bootloader)");
                    LOG_INFO("✅ Next reboot will show splash screen!");
                    LOG_INFO("   (Staying in Gateway mode for this session)");
                    LOG_INFO("════════════════════════════════════════════════════════");
                } else {
                    LOG_ERROR("❌ Failed to set boot partition: %d", err);
                }
            }
        } else {
            LOG_DEBUG("✅ Boot flow OK: Running from OTA_%d, Boot partition is OTA_%d", 
                     running_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0,
                     boot_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0);
        }
    }
#endif
```

**Complete context (lines 522-573):**
```cpp
    initSPI();

    OSThread::setup();

#ifdef ARCH_ESP32
    // ↑↑↑ INSERT THE ENTIRE BLOCK ABOVE HERE ↑↑↑
#endif

#if defined(ELECROW_ThinkNode_M1) || defined(ELECROW_ThinkNode_M2)
    // The ThinkNodes have their own blink logic
    // ledPeriodic = new Periodic("Blink", elecrowLedBlinker);
#else
    ledPeriodic = new Periodic("Blink", ledBlinker);
#endif
```

---

## 🔍 Why This Modification is Required

### The Problem:
When you flash firmware to ESP32 with an OTA swap partition table:
1. Fresh flash → ESP32 ROM bootloader defaults to **OTA_1** (Gateway)
2. Gateway boots directly, skipping the User Bootloader (OTA_0)
3. User never sees the splash screen!

### The Solution:
Gateway firmware detects this situation on **first boot only** and calls:
```cpp
esp_ota_set_boot_partition(OTA_0);
```

This tells the ESP32 bootloader: "From now on, always boot OTA_0 first."

### Boot Flow After Fix:
```
Flash → OTA_1 (Gateway) boots
     → Detects boot partition = OTA_1
     → Sets boot partition to OTA_0
     → Continues running (no restart)

Next reboot → OTA_0 (User Bootloader) runs
           → Shows splash screen
           → Sets boot to OTA_1
           → Launches Gateway
           
All future reboots → OTA_0 splash → OTA_1 Gateway (cycle repeats)
```

---

## 🔄 Applying to Fresh Meshtastic Firmware

If you need to update to a newer Meshtastic version:

### Step 1: Copy the custom variant
```bash
cp -r Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram \
      NEW_MESHTASTIC/firmware/variants/esp32/diy/
```

### Step 2: Apply main.cpp modifications

**Option A: Manual Edit**
1. Open `NEW_MESHTASTIC/firmware/src/main.cpp`
2. Find line with `#include "freertosinc.h"` (under `#ifdef ARCH_ESP32`)
3. Add the 2 header includes (see Change #1 above)
4. Find `OSThread::setup();`
5. Add the OTA swap detection block right after it (see Change #2 above)

**Option B: Use Patch File** (see below)

### Step 3: Rebuild
```bash
cd NEW_MESHTASTIC/firmware
pio run -e custom-sx1276-telegram-gateway
```

---

## 📄 Patch File

Create `meshtastic_ota_swap.patch`:

```patch
diff --git a/src/main.cpp b/src/main.cpp
index abc1234..def5678 100644
--- a/src/main.cpp
+++ b/src/main.cpp
@@ -46,6 +46,8 @@
 
 #ifdef ARCH_ESP32
 #include "freertosinc.h"
+#include "esp_ota_ops.h"
+#include "esp_partition.h"
 #if !MESHTASTIC_EXCLUDE_WEBSERVER
 #include "mesh/http/WebServer.h"
 #endif
@@ -522,6 +524,46 @@
 
     OSThread::setup();
 
+#ifdef ARCH_ESP32
+    // OTA Swap Bootloader: If we're running from OTA_1 but boot partition is also OTA_1,
+    // set boot partition to OTA_0 (User Bootloader) for next reboot.
+    // This ensures the splash screen shows on all subsequent boots.
+    const esp_partition_t *boot_partition = esp_ota_get_boot_partition();
+    const esp_partition_t *running_partition = esp_ota_get_running_partition();
+    
+    if (running_partition && boot_partition) {
+        LOG_DEBUG("🔍 Running from: subtype %d @ 0x%x", running_partition->subtype, running_partition->address);
+        LOG_DEBUG("🔍 Boot partition: subtype %d @ 0x%x", boot_partition->subtype, boot_partition->address);
+        
+        // If we're running from OTA_1 AND boot partition is OTA_1, switch boot to OTA_0
+        if (running_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1 &&
+            boot_partition->subtype == ESP_PARTITION_SUBTYPE_APP_OTA_1) {
+            
+            const esp_partition_t *ota0 = esp_partition_find_first(
+                ESP_PARTITION_TYPE_APP,
+                ESP_PARTITION_SUBTYPE_APP_OTA_0,
+                NULL
+            );
+            
+            if (ota0) {
+                esp_err_t err = esp_ota_set_boot_partition(ota0);
+                if (err == ESP_OK) {
+                    LOG_INFO("════════════════════════════════════════════════════════");
+                    LOG_INFO("✅ FIRST BOOT AFTER FLASH DETECTED!");
+                    LOG_INFO("✅ Boot partition set to OTA_0 (User Bootloader)");
+                    LOG_INFO("✅ Next reboot will show splash screen!");
+                    LOG_INFO("   (Staying in Gateway mode for this session)");
+                    LOG_INFO("════════════════════════════════════════════════════════");
+                } else {
+                    LOG_ERROR("❌ Failed to set boot partition: %d", err);
+                }
+            }
+        } else {
+            LOG_DEBUG("✅ Boot flow OK: Running from OTA_%d, Boot partition is OTA_%d", 
+                     running_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0,
+                     boot_partition->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0);
+        }
+    }
+#endif
+
 #if defined(ELECROW_ThinkNode_M1) || defined(ELECROW_ThinkNode_M2)
     // The ThinkNodes have their own blink logic
```

**Apply patch:**
```bash
cd NEW_MESHTASTIC/firmware
patch -p1 < meshtastic_ota_swap.patch
```

---

## 🧪 Testing the Modification

After applying changes and building:

### First Boot (right after flash):
```
INFO  | ════════════════════════════════════════════════════════
INFO  | ✅ FIRST BOOT AFTER FLASH DETECTED!
INFO  | ✅ Boot partition set to OTA_0 (User Bootloader)
INFO  | ✅ Next reboot will show splash screen!
INFO  |    (Staying in Gateway mode for this session)
INFO  | ════════════════════════════════════════════════════════
```

### Second Boot (after reset):
You should see the User Bootloader splash:
```
╔════════════════════════════════════════════════════════╗
║     🛰️  Meshtastic-Telegram Gateway v2.0             ║
╚════════════════════════════════════════════════════════╝

[1/3] User Bootloader started           ✅
[2/3] Locating Gateway firmware...      ✅  
[3/3] Validating firmware...            ✅
```

Then Gateway boots and shows:
```
DEBUG | 🔍 Running from: subtype 17 @ 0x100000
DEBUG | 🔍 Boot partition: subtype 16 @ 0x10000
DEBUG | ✅ Boot flow OK: Running from OTA_1, Boot partition is OTA_0
```

---

## ⚠️ Impact Analysis

### Does this break standard Meshtastic builds?
**NO.** The code is:
- Wrapped in `#ifdef ARCH_ESP32` (only affects ESP32 platform)
- Only activates when boot partition = running partition (rare case)
- Harmless for normal single-partition builds
- Runs once and exits on first boot

### Does this affect OTA updates?
**NO.** OTA updates continue to work normally:
- Updates flash to OTA_1 (Gateway)
- Boot partition remains OTA_0
- Next reboot: OTA_0 → new OTA_1 (standard flow)

### Side effects?
- Adds ~500 bytes to firmware size
- Runs once per flash (negligible performance impact)
- Creates helpful debug output for troubleshooting

---

## 📚 Related Documentation

- [OTA_SWAP_README.md](./OTA_SWAP_README.md) - Full OTA swap architecture
- [USER_BOOTLOADER_README.md](./USER_BOOTLOADER_README.md) - User Bootloader details
- [README.md](./README.md) - Main project documentation

---

## 🆘 Troubleshooting

### "I don't see the splash screen after reboot"
- Check serial output for "FIRST BOOT AFTER FLASH DETECTED" message
- If missing, the OTA swap code didn't run or failed
- Verify the code is after `OSThread::setup()`

### "Gateway always boots directly"
- Boot partition wasn't set to OTA_0
- Check logs for "Failed to set boot partition: X" error
- Verify User Bootloader is flashed to 0x10000

### "Build fails with 'esp_ota_ops.h not found'"
- You're on a non-ESP32 platform (nrf52, rp2040, etc.)
- The `#ifdef ARCH_ESP32` guard should prevent this
- Check that the header includes are inside the `#ifdef ARCH_ESP32` block

---

**Last Updated:** 2025-11-04  
**Status:** ✅ Production Ready

