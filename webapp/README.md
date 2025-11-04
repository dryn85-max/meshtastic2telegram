# 🌐 Meshtastic Gateway Web App

Telegram Web App for configuring the Meshtastic Telegram Gateway directly from Telegram.

## 📱 Features

- ✅ Configure WiFi credentials (SSID and Password)
- ✅ Set Telegram Bot Token and Chat ID
- ✅ Configure LoRa Region and Modem Preset
- ✅ Beautiful mobile-friendly interface
- ✅ Integrated with Telegram theme
- ✅ Real-time configuration updates
- ✅ Saves configuration to NVS (Non-Volatile Storage)

## 🚀 Deployment

This web app is automatically deployed to [https://dryn85-max.github.io/](https://dryn85-max.github.io/) via GitHub Actions.

Every push to `webapp/` directory in the `meshtastic2.0` repo triggers automatic deployment.

## 🔧 Setup

### 1. Create GitHub Personal Access Token (PAT)

1. Go to GitHub Settings → Developer settings → Personal access tokens → Tokens (classic)
2. Click "Generate new token (classic)"
3. Name: `Meshtastic WebApp Deploy`
4. Scopes: Select `repo` (Full control of private repositories)
5. Click "Generate token"
6. **Copy the token** (you won't see it again!)

### 2. Add Token to Repository Secrets

1. Go to `https://github.com/dryn85-max/meshtastic2.0/settings/secrets/actions`
2. Click "New repository secret"
3. Name: `GH_PAT`
4. Value: Paste your Personal Access Token
5. Click "Add secret"

### 3. Deploy

Push any changes to `webapp/` directory:

```bash
git add webapp/
git commit -m "Update web app"
git push origin main
```

GitHub Actions will automatically:
1. Detect changes in `webapp/` directory
2. Copy files to `dryn85-max.github.io` repo
3. Commit and push to GitHub Pages
4. Website updates within ~1 minute

## 📂 Files

- `index.html` - Main web app interface
- `styles.css` - Telegram-themed styling
- `app.js` - Configuration logic and Telegram Web App API integration

## 🔗 Integration with Telegram Bot

### Register Web App

Talk to @BotFather on Telegram:

```
/mybots
→ Select your bot
→ Bot Settings
→ Menu Button
→ Edit menu button URL
→ Enter: https://dryn85-max.github.io/
```

### Add /config Command

In @BotFather:

```
/setcommands
→ Select your bot
→ Enter commands:

config - Configure gateway settings
nodes - List mesh nodes
map - Show nodes on map
```

Now users can:
1. Send `/config` to bot
2. Web app opens in Telegram
3. Configure settings
4. Save → ESP32 receives configuration

## 🧪 Testing

### Test Locally

```bash
cd webapp/
python3 -m http.server 8000
```

Open: `http://localhost:8000`

### Test in Telegram

1. Use ngrok or similar to expose local server
2. Set webhook URL in @BotFather temporarily
3. Test configuration flow

## 📡 ESP32 Integration

The ESP32 firmware needs to handle messages from the web app:

### Required Commands

```cpp
// In TelegramModule.cpp
/config      → Open web app
get_status   → Return current configuration
save_config  → Save new configuration and reboot
```

### Message Format

```json
{
  "action": "save_config",
  "wifi_ssid": "MyNetwork",
  "wifi_password": "password123",
  "bot_token": "123456:ABC-DEF...",
  "chat_id": "987654321",
  "lora_region": 3,
  "lora_modem": 0
}
```

**Note:** The gateway firmware runs in a single mode (full Meshtastic mesh + Telegram). There is no mode switching in the current implementation.

## 🎨 Customization

The web app uses Telegram's theme colors automatically:
- Light/Dark theme detection
- Telegram button colors
- Telegram hint colors

To customize:
- Edit `styles.css` for visual changes
- Edit `app.js` for behavior changes

## 🔒 Security

- Passwords are hidden by default (toggle visibility with 👁️ button)
- Configuration sent directly to bot (not stored on server)
- GitHub Pages serves over HTTPS
- Telegram Web App provides user verification

## 📚 Resources

- [Telegram Web Apps Documentation](https://core.telegram.org/bots/webapps)
- [GitHub Pages Documentation](https://docs.github.com/en/pages)
- [Telegram Bot API](https://core.telegram.org/bots/api)

---

**Auto-deployed from:** `https://github.com/dryn85-max/meshtastic2.0`  
**Live at:** `https://dryn85-max.github.io/`

