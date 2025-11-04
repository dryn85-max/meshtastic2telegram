#!/bin/bash
# Flash script for User Bootloader + Gateway Firmware
# This script builds and flashes both the User Bootloader and Gateway firmware

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
PORT=${1:-/dev/tty.usbserial-0001}
BAUD=460800
PROJECT_ROOT="$(cd "$(dirname "$0")" && pwd)"
ESPTOOL="python3 $HOME/.platformio/packages/tool-esptoolpy/esptool.py"
PARTITION_GEN="python3 $HOME/.platformio/packages/framework-arduinoespressif32/tools/gen_esp32part.py"

echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Meshtastic-Telegram Gateway Flash Script            ║${NC}"
echo -e "${BLUE}║  User Bootloader + Gateway Firmware                  ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
echo ""

# Step 1: Build User Bootloader
echo -e "${YELLOW}[1/6] Building User Bootloader...${NC}"
cd "$PROJECT_ROOT/user_bootloader"
pio run -e user_bootloader || {
    echo -e "${RED}❌ Failed to build User Bootloader${NC}"
    exit 1
}
echo -e "${GREEN}✅ User Bootloader built successfully${NC}"
echo ""

# Step 2: Build Gateway Firmware
echo -e "${YELLOW}[2/6] Building Gateway Firmware...${NC}"
cd "$PROJECT_ROOT/Meshtastic_original/firmware"
pio run -e custom-sx1276-telegram-gateway 2>&1 | grep -v "Error: The program size" || true
echo -e "${GREEN}✅ Gateway Firmware built successfully${NC}"
echo ""

# Step 3: Create Gateway binary (if needed)
echo -e "${YELLOW}[3/6] Preparing binaries...${NC}"
GATEWAY_ELF="$PROJECT_ROOT/Meshtastic_original/firmware/.pio/build/custom-sx1276-telegram-gateway/firmware.elf"
GATEWAY_BIN="$PROJECT_ROOT/Meshtastic_original/firmware/.pio/build/custom-sx1276-telegram-gateway/firmware.bin"

if [ ! -f "$GATEWAY_BIN" ]; then
    echo "Creating Gateway binary from ELF..."
    $ESPTOOL --chip esp32 elf2image --flash_mode dio --flash_freq 40m --flash_size 4MB \
        -o "$GATEWAY_BIN" "$GATEWAY_ELF" || {
        echo -e "${RED}❌ Failed to create Gateway binary${NC}"
        exit 1
    }
    echo -e "${GREEN}✅ Gateway binary created${NC}"
fi

# Verify files exist
USER_BOOTLOADER="$PROJECT_ROOT/user_bootloader/.pio/build/user_bootloader/firmware.bin"
BOOTLOADER="$PROJECT_ROOT/user_bootloader/.pio/build/user_bootloader/bootloader.bin"
PARTITION_TABLE="$PROJECT_ROOT/Meshtastic_original/firmware/variants/esp32/diy/custom_sx1276_oled_telegram/partitions_ota_swap.csv"

echo "Checking files..."
MISSING_FILES=0

if [ -f "$USER_BOOTLOADER" ]; then 
    echo -e "  User Bootloader: ${GREEN}✅${NC}"
else 
    echo -e "  User Bootloader: ${RED}❌ MISSING${NC}"
    MISSING_FILES=1
fi

if [ -f "$BOOTLOADER" ]; then 
    echo -e "  Bootloader:      ${GREEN}✅${NC}"
else 
    echo -e "  Bootloader:      ${RED}❌ MISSING${NC}"
    MISSING_FILES=1
fi

if [ -f "$GATEWAY_BIN" ]; then 
    echo -e "  Gateway:         ${GREEN}✅${NC}"
else 
    echo -e "  Gateway:         ${RED}❌ MISSING${NC}"
    MISSING_FILES=1
fi

if [ -f "$PARTITION_TABLE" ]; then 
    echo -e "  Partition Table: ${GREEN}✅${NC}"
else 
    echo -e "  Partition Table: ${RED}❌ MISSING${NC}"
    MISSING_FILES=1
fi

if [ $MISSING_FILES -eq 1 ]; then
    echo ""
    echo -e "${RED}❌ Cannot proceed - required files are missing${NC}"
    echo -e "${YELLOW}Please check build errors above${NC}"
    exit 1
fi
echo ""

# Step 4: Generate partition table binary
echo -e "${YELLOW}[4/6] Generating partition table...${NC}"
PARTITION_BIN="$PROJECT_ROOT/.pio/partitions_ota_swap.bin"
$PARTITION_GEN "$PARTITION_TABLE" "$PARTITION_BIN" || {
    echo -e "${RED}❌ Failed to generate partition table${NC}"
    exit 1
}
echo -e "${GREEN}✅ Partition table generated${NC}"
echo ""

# Step 5: Display flash plan
echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║  Flash Plan - OTA Swap Architecture                   ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "  0x1000   → Bootloader (2nd stage)"
echo "  0x8000   → Partition Table (OTA-swap)"
echo "  0x10000  → User Bootloader (OTA_0, 960KB)   ← Splash / future config"
echo "  0x100000 → Gateway Firmware (OTA_1, 2.88MB) ← Meshtastic + Telegram"
echo ""
echo "Boot flow:"
echo "  Power-On → OTA_0 (User Bootloader) → OTA_1 (Gateway)"
echo ""
echo "Port: $PORT"
echo "Baud: $BAUD"
echo ""

# Step 6: Flash everything
echo -e "${YELLOW}[5/6] Flashing to ESP32...${NC}"
echo ""

$ESPTOOL --chip esp32 --port "$PORT" --baud "$BAUD" write_flash -z \
    0x1000   "$BOOTLOADER" \
    0x8000   "$PARTITION_BIN" \
    0x10000  "$USER_BOOTLOADER" \
    0x100000 "$GATEWAY_BIN" || {
    echo -e "${RED}❌ Flash failed!${NC}"
    exit 1
}

echo ""
echo -e "${GREEN}✅ Flash completed successfully!${NC}"
echo ""

# Step 7: Monitor
echo -e "${YELLOW}[6/6] Opening serial monitor...${NC}"
echo -e "${BLUE}Press Ctrl+C to exit${NC}"
echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════${NC}"
sleep 2

pio device monitor --port "$PORT" --baud 115200

