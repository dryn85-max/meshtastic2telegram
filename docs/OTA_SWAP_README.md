# OTA-Swap User Bootloader Architecture

**Status:** ✅ **IMPLEMENTED – IN PRODUCTION USE**

A boot flow that works *with* the ESP32 bootloader instead of fighting it.

---

## 💡 **Core Idea**

1. Flash a lightweight **User Bootloader** to **OTA_0** (`0x10000`).
2. Flash the full **Gateway firmware** to **OTA_1** (`0x100000`).
3. Let the Gateway, on its first run, set the boot partition to OTA_0.
4. Every subsequent boot starts in OTA_0 (splash + diagnostics) and then chains into OTA_1.

**Result:** Reliable splash + diagnostics before launching the full Meshtastic + Telegram gateway.

---

## 🎯 **Boot Flow**

```
Power-On
    ↓
ESP32 ROM bootloader reads OTA data (defaults to OTA_1 right after flashing)
    ↓
If boot partition == OTA_1 (first run): Gateway boots directly, sets boot partition to OTA_0, stays running
    ↓
Next reboot:
    OTA data points to OTA_0 → User Bootloader executes
        ├─ Shows welcome banner + hardware summary
        ├─ Validates Gateway image in OTA_1
        └─ Boots Gateway (OTA_1)
    ↓
Gateway operates normally (Meshtastic + Telegram)
```

---

## 📊 **Partition Layout (partitions_ota_swap.csv)**

```csv
# Name,     Type, SubType, Offset,   Size,     Purpose
nvs,        data, nvs,     0x009000, 0x005000, # 20KB - NVS storage
otadata,    data, ota,     0x00E000, 0x002000, # 8KB  - OTA metadata
ota_0,      app,  ota_0,   0x010000, 0x0F0000, # 960KB - User Bootloader (Arduino)
ota_1,      app,  ota_1,   0x100000, 0x2E0000, # 2.88MB - Gateway firmware
spiffs,     data, spiffs,  0x3E0000, 0x020000, # 128KB - Future storage / config UI
```

**Highlights:**
- OTA_0 has plenty of headroom for AP + web UI logic (✅ implemented in v1.1.0)
- OTA_1 comfortably hosts the ~1.7MB Gateway build (increased to 3MB partition)
- Factory partition removed – no more "boot selector" ROM hacks
- **Simplified to 2 firmwares only** - User Bootloader (OTA_0) + Gateway (OTA_1)

---

## 🚀 **Installation**

### Automated script

```bash
./flash_complete_system.sh /dev/tty.usbserial-0001
```

The script builds both projects, generates the partition table and flashes:
- `0x1000`   – 2nd stage bootloader (from user_bootloader build)
- `0x8000`   – OTA-swap partition table
- `0x10000`  – User Bootloader (OTA_0)
- `0x100000` – Gateway firmware (OTA_1)

### Manual commands

See updated instructions in the main `README.md`.

---

## 📺 **Runtime Behaviour**

1. **Immediately after flashing:**
   - ROM bootloader boots OTA_1 → Gateway detects it is running “bare”.
   - Gateway calls `esp_ota_set_boot_partition(OTA_0)` and logs:
     > “Next reboot will show the User Bootloader splash! (Staying in Gateway mode for this session)”
   - No forced restart – Gateway continues to run.

2. **After first manual reboot / power cycle:**
   - User Bootloader (OTA_0) runs, prints splash, validates Gateway, then boots OTA_1.
   - From now on every reboot shows the splash first.

3. **Normal cycle:**
   - Splash → Gateway logs → Meshtastic + Telegram gateway ready.

---

## 🔄 **Over-the-Air Updates**

- OTA updates still target **OTA_1** (Gateway). The User Bootloader never gets overwritten.
- After a successful OTA update to OTA_1, the boot partition remains OTA_0.
- A reboot follows the usual flow: OTA_0 splash → new Gateway.

---

## 💾 **Memory Usage**

| Component        | Flash Size | Partition | Headroom |
|------------------|------------|-----------|----------|
| User Bootloader  | ~273 KB    | 960 KB    | ~687 KB  |
| Gateway Firmware | ~1.7 MB    | 2.88 MB   | ~1.18 MB |

SRAM usage is unchanged (~173 KB free in Gateway mode).

---

## 🧪 Troubleshooting

- **Gateway keeps printing “First boot after flash”**
  - Ensure you flashed both OTA_0 and OTA_1 from the latest build.
  - After the first power cycle you should see the splash; if not, gather serial logs.

- **Splash appears but Gateway fails to launch**
  - User Bootloader will log any validation failures (size, checksum). Reflash OTA_1.

- **OTA update bricks Gateway**
  - User Bootloader remains intact. Reflash only the OTA_1 image.

---

## 🛠️ Next Steps (Roadmap)

- Add **AP + web config UI** to the User Bootloader (makes use of the 960KB partition).
- Allow holding BOOT for 3s to drop into config mode.
- Persist WiFi/Telegram/LoRa settings via NVS from the web page.
- Add sanity checks / CRC for OTA_1 image (optional).

---

## 📁 Related Files

- `user_bootloader/` – Arduino project for OTA_0 splash + launch logic
- `Meshtastic_original/firmware/` – Main Gateway project (OTA_1)
- `flash_complete_system.sh` – Complete system build + flash script
- `partitions_ota_swap.csv` – Partition layout

For historical context on the abandoned "Boot Selector (Factory)" approach, see [`docs/MULTIBOOT_README.md`](MULTIBOOT_README.md).

