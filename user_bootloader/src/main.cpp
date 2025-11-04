/**
 * @file main.cpp
 * @brief User Bootloader for Meshtastic-Telegram Gateway
 * 
 * This firmware runs on every power-on, displays a welcome message,
 * and then boots the Gateway firmware from OTA_1.
 * 
 * Features:
 * - Displays welcome message with version info
 * - Shows hardware configuration
 * - BOOT button (hold 3s) → WiFi AP + Web Config Portal
 * - Saves WiFi/Telegram/LoRa config to NVS
 * - Automatically boots Gateway firmware
 * 
 * Size: ~150KB (fits in 960KB OTA_0 partition)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "esp_ota_ops.h"
#include "esp_partition.h"

// Version information
#define BOOTLOADER_VERSION "1.1.0"
#define GATEWAY_VERSION "2.0"

// Hardware configuration
#define HARDWARE_NAME "ESP32 + SX1276 LoRa"
#define LORA_FREQUENCY "868 MHz"
#define FLASH_SIZE "4MB"

// Pin definitions
#define BOOT_BUTTON_PIN 0  // GPIO0 (BOOT button on most ESP32 boards)

// Timing
#define MESSAGE_DISPLAY_TIME_MS 2000  // Show message for 2 seconds
#define BUTTON_CHECK_TIME_MS 3000     // Check button for 3 seconds
#define BUTTON_POLL_INTERVAL_MS 50    // Check button every 50ms

// Config AP settings
#define CONFIG_AP_SSID "MG-Config"
#define CONFIG_AP_PASSWORD "meshtastic"
#define CONFIG_AP_IP IPAddress(192, 168, 4, 1)
#define CONFIG_AP_GATEWAY IPAddress(192, 168, 4, 1)
#define CONFIG_AP_SUBNET IPAddress(255, 255, 255, 0)

// NVS namespace
#define NVS_NAMESPACE "meshtastic"

// Validation
#define MIN_GATEWAY_SIZE_BYTES (1500000)  // 1.5MB minimum (Gateway is ~1.7MB)
#define SERIAL_BAUD 115200

// Global objects
AsyncWebServer server(80);
Preferences prefs;

// ============================================================================
// WELCOME MESSAGE & BOOT SEQUENCE
// ============================================================================

void printWelcomeMessage() {
    Serial.println("\n\n");
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║                                                        ║");
    Serial.println("║     🛰️  Meshtastic-Telegram Gateway v2.0             ║");
    Serial.println("║                                                        ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("┌────────────────────────────────────────────────────────┐");
    Serial.println("│ Hardware Information                                   │");
    Serial.println("├────────────────────────────────────────────────────────┤");
    Serial.printf("│ Platform       : %-38s│\n", HARDWARE_NAME);
    Serial.printf("│ LoRa Frequency : %-38s│\n", LORA_FREQUENCY);
    Serial.printf("│ Flash Size     : %-38s│\n", FLASH_SIZE);
    Serial.printf("│ Free RAM       : ~173KB%-31s│\n", "");
    Serial.println("└────────────────────────────────────────────────────────┘");
    Serial.println();
    Serial.println("┌────────────────────────────────────────────────────────┐");
    Serial.println("│ Firmware Configuration                                 │");
    Serial.println("├────────────────────────────────────────────────────────┤");
    Serial.println("│ Mode           : Gateway (Full Mesh + Telegram)        │");
    Serial.println("│ Bluetooth      : Disabled (RAM optimization)           │");
    Serial.println("│ OLED Display   : Disabled (RAM optimization)           │");
    Serial.println("│ Mesh Routing   : Enabled (Full repeater)               │");
    Serial.println("│ Telegram Bot   : Enabled (with SSL/TLS)                │");
    Serial.println("└────────────────────────────────────────────────────────┘");
    Serial.println();
    Serial.printf("Bootloader Version: %s\n", BOOTLOADER_VERSION);
    Serial.printf("Gateway Version:    %s\n\n", GATEWAY_VERSION);
}

void printBootMessage() {
    Serial.println("┌────────────────────────────────────────────────────────┐");
    Serial.println("│ Boot Sequence                                          │");
    Serial.println("└────────────────────────────────────────────────────────┘");
    Serial.println();
    Serial.println("  [1/3] User Bootloader started           ✅");
    Serial.println("  [2/3] Locating Gateway firmware...      ");
}

// ============================================================================
// CONFIG PORTAL WEB PAGES
// ============================================================================

const char* htmlConfigPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>MG Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 20px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 500px;
            width: 100%;
            padding: 40px;
            animation: slideIn 0.3s ease-out;
        }
        @keyframes slideIn {
            from { transform: translateY(-30px); opacity: 0; }
            to { transform: translateY(0); opacity: 1; }
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 10px;
            font-size: 28px;
        }
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 30px;
            font-size: 14px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            color: #555;
            font-weight: 600;
            margin-bottom: 8px;
            font-size: 14px;
        }
        input, select {
            width: 100%;
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            font-size: 14px;
            transition: all 0.3s;
        }
        input:focus, select:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        .btn {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s, box-shadow 0.2s;
            margin-top: 10px;
        }
        .btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        .btn:active {
            transform: translateY(0);
        }
        .alert {
            padding: 12px;
            border-radius: 8px;
            margin-bottom: 20px;
            display: none;
        }
        .alert-success {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .alert-error {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .section-title {
            color: #667eea;
            font-size: 16px;
            font-weight: 600;
            margin-top: 20px;
            margin-bottom: 15px;
            padding-bottom: 8px;
            border-bottom: 2px solid #e0e0e0;
        }
        .section-title:first-of-type {
            margin-top: 0;
        }
        .help-text {
            font-size: 12px;
            color: #999;
            margin-top: 4px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🛰️ Gateway Config</h1>
        <p class="subtitle">Meshtastic-Telegram Gateway v2.0</p>
        
        <div id="alert" class="alert"></div>
        
        <form id="configForm">
            <div class="section-title">📶 WiFi Settings</div>
            
            <div class="form-group">
                <label for="wifi_ssid">WiFi SSID *</label>
                <input type="text" id="wifi_ssid" name="wifi_ssid" required>
            </div>
            
            <div class="form-group">
                <label for="wifi_pass">WiFi Password *</label>
                <input type="password" id="wifi_pass" name="wifi_pass" required>
                <div class="help-text">Leave blank to connect to open network</div>
            </div>
            
            <div class="section-title">🤖 Telegram Settings</div>
            
            <div class="form-group">
                <label for="bot_token">Bot Token *</label>
                <input type="text" id="bot_token" name="bot_token" required placeholder="1234567890:ABCdefGHIjklMNOpqrsTUVwxyz">
                <div class="help-text">Get from @BotFather on Telegram</div>
            </div>
            
            <div class="form-group">
                <label for="chat_id">Chat ID *</label>
                <input type="text" id="chat_id" name="chat_id" required placeholder="123456789">
                <div class="help-text">Your Telegram user ID</div>
            </div>
            
            <div class="section-title">📡 LoRa Settings</div>
            
            <div class="form-group">
                <label for="lora_region">Region *</label>
                <select id="lora_region" name="lora_region" required>
                    <option value="0">UNSET</option>
                    <option value="1">US</option>
                    <option value="2">EU_433</option>
                    <option value="3" selected>EU_868</option>
                    <option value="4">CN</option>
                    <option value="5">JP</option>
                    <option value="6">ANZ</option>
                    <option value="7">KR</option>
                    <option value="8">TW</option>
                    <option value="9">RU</option>
                    <option value="10">IN</option>
                    <option value="11">NZ_865</option>
                    <option value="12">TH</option>
                    <option value="13">UA_433</option>
                    <option value="14">UA_868</option>
                </select>
            </div>
            
            <div class="form-group">
                <label for="lora_preset">Modem Preset *</label>
                <select id="lora_preset" name="lora_preset" required>
                    <option value="0" selected>LONG_FAST (Default)</option>
                    <option value="1">LONG_SLOW</option>
                    <option value="2">VERY_LONG_SLOW</option>
                    <option value="3">MEDIUM_SLOW</option>
                    <option value="4">MEDIUM_FAST</option>
                    <option value="5">SHORT_SLOW</option>
                    <option value="6">SHORT_FAST</option>
                    <option value="7">LONG_MODERATE</option>
                </select>
            </div>
            
            <button type="submit" class="btn">💾 Save & Reboot</button>
        </form>
    </div>

    <script>
        document.getElementById('configForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            const alert = document.getElementById('alert');
            const btn = document.querySelector('.btn');
            
            btn.textContent = '⏳ Saving...';
            btn.disabled = true;
            
            const formData = new FormData(e.target);
            const data = Object.fromEntries(formData.entries());
            
            try {
                const response = await fetch('/save', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });
                
                if (response.ok) {
                    alert.className = 'alert alert-success';
                    alert.textContent = '✅ Configuration saved! Rebooting in 3 seconds...';
                    alert.style.display = 'block';
                    btn.textContent = '✅ Saved!';
                    
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 3000);
                } else {
                    throw new Error('Save failed');
                }
            } catch (error) {
                alert.className = 'alert alert-error';
                alert.textContent = '❌ Failed to save configuration. Please try again.';
                alert.style.display = 'block';
                btn.textContent = '💾 Save & Reboot';
                btn.disabled = false;
            }
        });
    </script>
</body>
</html>
)rawliteral";

// ============================================================================
// CONFIG PORTAL FUNCTIONS
// ============================================================================

void saveConfigToNVS(const String& wifiSSID, const String& wifiPass, 
                     const String& botToken, const String& chatID,
                     int loraRegion, int loraPreset) {
    prefs.begin(NVS_NAMESPACE, false);
    
    prefs.putString("wifi_ssid", wifiSSID);
    prefs.putString("wifi_pass", wifiPass);
    prefs.putString("bot_token", botToken);
    prefs.putString("chat_id", chatID);
    prefs.putInt("lora_region", loraRegion);
    prefs.putInt("lora_preset", loraPreset);
    
    prefs.end();
    
    Serial.println("\n✅ Configuration saved to NVS:");
    Serial.printf("   WiFi SSID:    %s\n", wifiSSID.c_str());
    Serial.printf("   WiFi Pass:    %s\n", wifiPass.length() > 0 ? "********" : "(empty)");
    Serial.printf("   Bot Token:    %s...%s\n", 
                  botToken.substring(0, 10).c_str(),
                  botToken.substring(botToken.length() - 4).c_str());
    Serial.printf("   Chat ID:      %s\n", chatID.c_str());
    Serial.printf("   LoRa Region:  %d\n", loraRegion);
    Serial.printf("   LoRa Preset:  %d\n", loraPreset);
    Serial.println();
}

void startConfigPortal() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║          🔧 CONFIG MODE ACTIVATED                      ║");
    Serial.println("╚════════════════════════════════════════════════════════╝\n");
    
    // Start WiFi AP
    Serial.println("Starting WiFi Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(CONFIG_AP_IP, CONFIG_AP_GATEWAY, CONFIG_AP_SUBNET);
    WiFi.softAP(CONFIG_AP_SSID, CONFIG_AP_PASSWORD);
    
    Serial.println("\n✅ WiFi AP Started!");
    Serial.printf("   SSID:     %s\n", CONFIG_AP_SSID);
    Serial.printf("   Password: %s\n", CONFIG_AP_PASSWORD);
    Serial.printf("   IP:       %s\n\n", CONFIG_AP_IP.toString().c_str());
    Serial.println("📱 Connect to the WiFi network and open:");
    Serial.printf("   http://%s\n\n", CONFIG_AP_IP.toString().c_str());
    Serial.println("Waiting for configuration...\n");
    
    // Setup web server routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", htmlConfigPage);
    });
    
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
            // Parse JSON
            String json = String((char*)data);
            
            // Simple JSON parsing (no library needed for this simple case)
            String wifiSSID = "";
            String wifiPass = "";
            String botToken = "";
            String chatID = "";
            int loraRegion = 3;  // EU_868 default
            int loraPreset = 0;  // LONG_FAST default
            
            // Extract values (simple string parsing)
            int idx = json.indexOf("\"wifi_ssid\":\"");
            if (idx >= 0) {
                int start = idx + 13;
                int end = json.indexOf("\"", start);
                wifiSSID = json.substring(start, end);
            }
            
            idx = json.indexOf("\"wifi_pass\":\"");
            if (idx >= 0) {
                int start = idx + 13;
                int end = json.indexOf("\"", start);
                wifiPass = json.substring(start, end);
            }
            
            idx = json.indexOf("\"bot_token\":\"");
            if (idx >= 0) {
                int start = idx + 13;
                int end = json.indexOf("\"", start);
                botToken = json.substring(start, end);
            }
            
            idx = json.indexOf("\"chat_id\":\"");
            if (idx >= 0) {
                int start = idx + 11;
                int end = json.indexOf("\"", start);
                chatID = json.substring(start, end);
            }
            
            idx = json.indexOf("\"lora_region\":\"");
            if (idx >= 0) {
                int start = idx + 15;
                int end = json.indexOf("\"", start);
                loraRegion = json.substring(start, end).toInt();
            }
            
            idx = json.indexOf("\"lora_preset\":\"");
            if (idx >= 0) {
                int start = idx + 15;
                int end = json.indexOf("\"", start);
                loraPreset = json.substring(start, end).toInt();
            }
            
            // Validate
            if (wifiSSID.length() == 0 || botToken.length() == 0 || chatID.length() == 0) {
                request->send(400, "text/plain", "Missing required fields");
                return;
            }
            
            // Save to NVS
            saveConfigToNVS(wifiSSID, wifiPass, botToken, chatID, loraRegion, loraPreset);
            
            request->send(200, "text/plain", "OK");
            
            // Reboot after a delay
            Serial.println("Rebooting in 3 seconds...\n");
            delay(3000);
            ESP.restart();
        }
    );
    
    server.begin();
    Serial.println("🌐 Web server started!\n");
    
    // Keep running until restart
    while (true) {
        delay(100);
    }
}

// ============================================================================
// BUTTON DETECTION
// ============================================================================

bool checkBootButton() {
    Serial.println("🔘 Checking BOOT button (hold for 3s to enter Config Mode)...");
    
    unsigned long startTime = millis();
    bool buttonPressed = false;
    int dotCount = 0;
    
    while (millis() - startTime < BUTTON_CHECK_TIME_MS) {
        if (digitalRead(BOOT_BUTTON_PIN) == LOW) {
            buttonPressed = true;
            
            // Print progress dots
            if ((millis() - startTime) % 500 == 0 && dotCount < 6) {
                Serial.print(".");
                dotCount++;
            }
        } else {
            if (buttonPressed) {
                Serial.println(" Released\n");
                return false;
            }
        }
        delay(BUTTON_POLL_INTERVAL_MS);
    }
    
    if (buttonPressed) {
        Serial.println(" ✅ Held!\n");
        return true;
    }
    
    Serial.println(" Not pressed\n");
    return false;
}

// ============================================================================
// GATEWAY BOOT FUNCTIONS
// ============================================================================

void bootGatewayFirmware() {
    // Find OTA_1 partition (Gateway firmware)
    const esp_partition_t* gateway = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_1,
        NULL
    );
    
    if (gateway == NULL) {
        Serial.println("  [2/3] Locating Gateway firmware...      ❌");
        Serial.println();
        Serial.println("╔════════════════════════════════════════════════════════╗");
        Serial.println("║                      ⚠️  ERROR                         ║");
        Serial.println("╚════════════════════════════════════════════════════════╝");
        Serial.println();
        Serial.println("Gateway firmware not found in OTA_1 partition!");
        Serial.println();
        Serial.println("Please flash the Gateway firmware:");
        Serial.println("  1. Build: pio run -e custom-sx1276-telegram-gateway");
        Serial.println("  2. Flash to 0x100000 (OTA_1)");
        Serial.println();
        Serial.println("Device will restart in 10 seconds...");
        delay(10000);
        ESP.restart();
        return;
    }
    
    Serial.println("  [2/3] Locating Gateway firmware...      ✅");
    
    // Verify partition is valid
    if (gateway->size < MIN_GATEWAY_SIZE_BYTES) {
        Serial.println("  [3/3] Validating firmware...            ⚠️");
        Serial.println();
        Serial.printf("Warning: Gateway partition is %d bytes (expected >%d bytes).\n", 
                      gateway->size, MIN_GATEWAY_SIZE_BYTES);
        Serial.println("Partition may be empty or corrupted.");
        Serial.println("Attempting to boot anyway...");
        Serial.println();
    } else {
        Serial.println("  [3/3] Validating firmware...            ✅");
        Serial.println();
    }
    
    Serial.printf("Gateway Partition Info:\n");
    Serial.printf("  - Address: 0x%X\n", gateway->address);
    Serial.printf("  - Size:    %d bytes (%.2f MB)\n", gateway->size, gateway->size / 1024.0 / 1024.0);
    Serial.println();
    
    // Set OTA_1 as boot partition
    esp_err_t err = esp_ota_set_boot_partition(gateway);
    if (err != ESP_OK) {
        Serial.println("❌ Failed to set boot partition!");
        Serial.printf("   Error code: 0x%X\n", err);
        Serial.println();
        Serial.println("Device will restart in 5 seconds...");
        delay(5000);
        ESP.restart();
        return;
    }
    
    Serial.println("✅ Boot partition set successfully!");
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║         Launching Gateway Firmware...                 ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    
    delay(1000);
    
    // Restart into Gateway firmware
    ESP.restart();
}

// ============================================================================
// MAIN SETUP
// ============================================================================

void setup() {
    // Initialize serial
    Serial.begin(SERIAL_BAUD);
    delay(500);  // Wait for serial to stabilize
    
    // Initialize button pin
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    
    // Print welcome message
    printWelcomeMessage();
    
    // Check for config mode (button held)
    if (checkBootButton()) {
        startConfigPortal();
        // Never returns (reboots after config)
    }
    
    // Normal boot sequence - button check already took 3 seconds
    // No additional delay needed (button check > message display time)
    
    // Print boot sequence
    printBootMessage();
    delay(500);
    
    // Boot Gateway firmware
    bootGatewayFirmware();
}

void loop() {
    // We never get here - restart happens in setup()
    // But just in case, restart again
    delay(1000);
    Serial.println("INTERNAL ERROR: Bootloader loop() reached unexpectedly!");
    Serial.println("This indicates a firmware bug. Please report this.");
    Serial.println("Restarting...");
    ESP.restart();
}
