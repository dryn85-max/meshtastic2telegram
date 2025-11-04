# Hardware Pinout Documentation

## ⚠️ Important: Current Firmware Configuration

**Gateway Firmware (`custom-sx1276-telegram-gateway`):**
- ✅ LoRa SX1276 - **ACTIVE**
- ❌ OLED Display - **DISABLED** (to save ~15KB RAM for SSL)
- ❌ Bluetooth/BLE - **DISABLED** (to save ~30KB RAM for SSL)
- ✅ Boot Button - **ACTIVE** (User Bootloader config portal trigger)
- ✅ LED - **ACTIVE**

**Configuration:** Via User Bootloader's WiFi AP + Web Portal (hold BOOT button for 3 seconds during power-on)

---

## Complete Pin Assignment

### ESP32 GPIO Pin Usage

| GPIO | Function | Direction | Notes | Gateway Firmware |
|------|----------|-----------|-------|-----------------|
| 0 | Button | Input | Boot button, has internal pull-up | ✅ Used |
| 2 | LED | Output | Built-in LED on most ESP32 boards | ✅ Used |
| 4 | I2C SDA | I/O | Reserved (OLED disabled) | ❌ Not used |
| 5 | SPI SCK | Output | LoRa SPI clock | ✅ Used |
| 14 | LoRa Reset | Output | SX1276 reset pin | ✅ Used |
| 15 | I2C SCL | Output | Reserved (OLED disabled) | ❌ Not used |
| 16 | OLED Reset | Output | Reserved (OLED disabled) | ❌ Not used |
| 18 | LoRa CS | Output | LoRa chip select | ✅ Used |
| 19 | SPI MISO | Input | LoRa SPI data in | ✅ Used |
| 26 | LoRa DIO0 | Input | LoRa IRQ/interrupt pin | ✅ Used |
| 27 | SPI MOSI | Output | LoRa SPI data out | ✅ Used |
| 32 | LoRa DIO2 | Input | Optional LoRa pin | ✅ Used |
| 33 | LoRa DIO1 | Input | Optional LoRa IRQ | ✅ Used |

### Reserved/Boot Pins (Not Used)

These pins have special functions during ESP32 boot and are not used in this design:

| GPIO | Boot Function | Notes |
|------|---------------|-------|
| 1 | UART0 TX | Serial console |
| 3 | UART0 RX | Serial console |
| 6-11 | Flash SPI | Connected to internal flash |
| 12 | Boot voltage | Must be low during boot for 3.3V flash |

### Available Pins for Future Expansion

| GPIO | Available | Notes |
|------|-----------|-------|
| 13 | Yes | Can be used for TX/RX control or other features |
| 21 | Yes | Standard I2C SDA (alternative) |
| 22 | Yes | Standard I2C SCL (alternative) |
| 23 | Yes | Available GPIO |
| 25 | Yes | DAC1, can be used for audio |
| 34-39 | Yes | Input only, no pull-up/down |
| 35 | Yes | ADC, good for battery voltage sensing |

## SSD1306 OLED Display

**⚠️ Status: DISABLED in Gateway Firmware**

The OLED display is **disabled** in the Gateway firmware (`custom-sx1276-telegram-gateway`) to save ~15KB RAM for SSL/TLS operations. The pins below are reserved for future use or custom modifications.

### Connection Details (if re-enabling OLED)

```
OLED Module    ESP32
-----------    -----
VCC            3.3V
GND            GND
SDA            GPIO 4
SCL            GPIO 15
RST            GPIO 16
```

### I2C Specifications
- **Protocol**: I2C
- **Address**: 0x3C (60 decimal)
- **Speed**: 400kHz (Fast Mode)
- **Resolution**: 128x64 pixels
- **Color**: Monochrome (White/Blue)

### Notes
- Some OLED modules may not have a RST pin - in that case, connect to 3.3V
- Ensure pull-up resistors are present on SDA/SCL (usually built into OLED module)
- Display can run on 3.3V or 5V VCC

## SX1276 LoRa Radio Module

### Connection Details

```
LoRa Module    ESP32
-----------    -----
VCC            3.3V
GND            GND
SCK            GPIO 5
MISO           GPIO 19
MOSI           GPIO 27
NSS/CS         GPIO 18
RESET          GPIO 14
DIO0           GPIO 26
DIO1           GPIO 33 (optional)
DIO2           GPIO 32 (optional)
```

### SPI Specifications
- **Protocol**: SPI
- **Speed**: 2MHz (default, can be increased)
- **Mode**: SPI Mode 0 (CPOL=0, CPHA=0)
- **Bit Order**: MSB First

### Frequency Bands
- **Current**: 868MHz (EU region)
- **Supported**: 433MHz, 868MHz, 915MHz (depends on module)

### Important Notes
- **Antenna**: MUST be connected before powering on
- **Power**: 3.3V only, do NOT use 5V
- **DIO0**: Required for interrupt-driven operation
- **DIO1/DIO2**: Optional but recommended for advanced features

## Power Supply

### Requirements
- **Input Voltage**: 5V via USB or Vin pin
- **ESP32 Current**: ~240mA (peak during WiFi/BT)
- **LoRa TX Current**: ~120mA @ max power
- **OLED Current**: ~20mA
- **Total Peak**: ~400mA recommended

### Power Pins
```
ESP32 Board    Power Rail
-----------    ----------
USB 5V         5V input
3V3            3.3V regulated output
GND            Ground
```

### Battery Operation (Optional)
For battery operation, consider:
- **Voltage**: 3.7V LiPo or 3x AA (3.6-4.5V range)
- **Capacity**: Minimum 2000mAh recommended
- **Charging**: Add TP4056 charging module
- **Voltage Sensing**: Connect battery through voltage divider to GPIO 35

## Schematic Diagram

```
                    ESP32-WROOM-32
                   ┌─────────────┐
                   │             │
    [OLED SDA]────┤ GPIO 4      │
    [OLED SCL]────┤ GPIO 15     │
    [OLED RST]────┤ GPIO 16     │
                   │             │
    [LoRa SCK]────┤ GPIO 5      │
    [LoRa MISO]───┤ GPIO 19     │
    [LoRa MOSI]───┤ GPIO 27     │
    [LoRa CS]─────┤ GPIO 18     │
    [LoRa RST]────┤ GPIO 14     │
    [LoRa DIO0]───┤ GPIO 26     │
    [LoRa DIO1]───┤ GPIO 33     │
    [LoRa DIO2]───┤ GPIO 32     │
                   │             │
    [Button]──────┤ GPIO 0      │
    [LED]─────────┤ GPIO 2      │
                   │             │
    [USB]─────────┤ USB-UART    │
                   └─────────────┘
```

## Assembly Notes

1. **Soldering**: Use 63/37 or 60/40 tin-lead solder
2. **Wire Gauge**: 22-26 AWG for connections
3. **Wire Length**: Keep I2C wires < 20cm, SPI wires < 30cm
4. **Antenna**: Use 868MHz quarter-wave antenna (~8.2cm)
5. **Mounting**: Secure OLED to prevent stress on solder joints

## Testing Procedure

1. **Visual Inspection**: Check all solder joints
2. **Continuity Test**: Verify connections with multimeter
3. **Power Test**: Apply 5V, measure 3.3V rail
4. **I2C Scan**: Use I2C scanner to verify OLED at 0x3C
5. **LoRa Test**: Upload firmware and check for initialization
6. **Display Test**: Verify OLED shows Meshtastic logo
7. **Radio Test**: Perform range test with another node

## Troubleshooting

### OLED Issues
- **No display**: Check 3.3V power, verify I2C address
- **Garbled display**: Check SDA/SCL connections, reduce I2C speed
- **Dim display**: Increase display brightness in firmware

### LoRa Issues
- **No transmission**: Check antenna connection, verify DIO0
- **Poor range**: Check antenna, verify frequency matches region
- **Module not found**: Verify SPI connections, check CS pin

### Power Issues
- **Brownouts**: Use better power supply, add capacitors
- **Resets during TX**: Add 100-220µF capacitor near LoRa module
- **Unstable operation**: Check all GND connections

## Safety Warnings

⚠️ **Important Safety Information**

1. **Never operate LoRa module without antenna**
   - Can damage the PA (power amplifier)
   - Use proper antenna for your frequency

2. **Voltage levels**
   - SX1276 is 3.3V only - do not use 5V
   - ESP32 GPIO are 3.3V - verify level compatibility

3. **RF exposure**
   - Keep antenna away from body during transmission
   - Follow local regulations for radio transmission

4. **Static protection**
   - Use ESD protection when handling modules
   - LoRa radio is sensitive to static discharge

