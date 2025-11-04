// Telegram Web App initialization
let tg = window.Telegram.WebApp;
tg.expand();
tg.enableClosingConfirmation();

// Apply Telegram theme
document.documentElement.style.setProperty('--tg-theme-bg-color', tg.backgroundColor || '#ffffff');
document.documentElement.style.setProperty('--tg-theme-text-color', tg.textColor || '#000000');
document.documentElement.style.setProperty('--tg-theme-hint-color', tg.hintColor || '#707579');
document.documentElement.style.setProperty('--tg-theme-link-color', tg.linkColor || '#3390ec');
document.documentElement.style.setProperty('--tg-theme-button-color', tg.buttonColor || '#3390ec');
document.documentElement.style.setProperty('--tg-theme-button-text-color', tg.buttonTextColor || '#ffffff');
document.documentElement.style.setProperty('--tg-theme-secondary-bg-color', tg.secondaryBackgroundColor || '#f4f4f5');

// Load current configuration
window.addEventListener('DOMContentLoaded', () => {
    loadCurrentConfig();
    setupEventListeners();
});

function setupEventListeners() {
    const modeSelect = document.getElementById('mode');
    const telegramConfig = document.getElementById('telegram-config');
    
    modeSelect.addEventListener('change', (e) => {
        if (e.target.value === 'TELEGRAM') {
            telegramConfig.style.display = 'block';
        } else {
            telegramConfig.style.display = 'none';
        }
    });
}

function loadCurrentConfig() {
    // Get current config from Telegram Web App init data
    const initData = tg.initDataUnsafe;
    
    // Request current status from bot
    tg.sendData(JSON.stringify({
        action: 'get_status'
    }));
    
    // For demo, set default values
    document.getElementById('current-mode').textContent = 'Loading...';
    
    // Simulate loading current config (will be replaced with actual bot response)
    setTimeout(() => {
        // This will be updated when we implement the bot response handler
        document.getElementById('current-mode').textContent = 'TELEGRAM Mode Active';
    }, 1000);
}

function togglePassword(fieldId) {
    const field = document.getElementById(fieldId);
    const button = event.target;
    
    if (field.type === 'password') {
        field.type = 'text';
        button.textContent = '🙈';
    } else {
        field.type = 'password';
        button.textContent = '👁️';
    }
}

async function saveConfig() {
    const mode = document.getElementById('mode').value;
    const config = {
        action: 'save_config',
        mode: mode
    };
    
    if (mode === 'TELEGRAM') {
        const wifiSsid = document.getElementById('wifi-ssid').value.trim();
        const wifiPassword = document.getElementById('wifi-password').value;
        const botToken = document.getElementById('bot-token').value.trim();
        const chatId = document.getElementById('chat-id').value.trim();
        
        // Validation
        if (!wifiSsid || !wifiPassword) {
            showError('Please enter WiFi credentials');
            return;
        }
        
        if (!botToken || !chatId) {
            showError('Please enter Telegram bot credentials');
            return;
        }
        
        config.wifi_ssid = wifiSsid;
        config.wifi_password = wifiPassword;
        config.bot_token = botToken;
        config.chat_id = chatId;
    }
    
    // Show loading
    showLoading();
    
    try {
        // Send configuration to bot
        // The bot will forward this to the ESP32 device
        tg.sendData(JSON.stringify(config));
        
        // Show success after a delay (simulating device response)
        setTimeout(() => {
            hideLoading();
            showSuccess();
            
            // Close the web app after 3 seconds
            setTimeout(() => {
                tg.close();
            }, 3000);
        }, 2000);
        
    } catch (error) {
        hideLoading();
        showError('Failed to save configuration: ' + error.message);
    }
}

function closeApp() {
    if (confirm('Close without saving?')) {
        tg.close();
    }
}

function showLoading() {
    document.getElementById('loading').classList.remove('hidden');
    document.getElementById('save-btn').disabled = true;
    document.getElementById('cancel-btn').disabled = true;
}

function hideLoading() {
    document.getElementById('loading').classList.add('hidden');
    document.getElementById('save-btn').disabled = false;
    document.getElementById('cancel-btn').disabled = false;
}

function showSuccess() {
    document.getElementById('success').classList.remove('hidden');
    setTimeout(() => {
        document.getElementById('success').classList.add('hidden');
    }, 5000);
}

function showError(message) {
    document.getElementById('error-message').textContent = message;
    document.getElementById('error').classList.remove('hidden');
    setTimeout(() => {
        document.getElementById('error').classList.add('hidden');
    }, 5000);
}

// Handle messages from Telegram Bot (status updates)
window.addEventListener('message', (event) => {
    if (event.data && event.data.type === 'config_status') {
        const status = event.data.data;
        
        // Update current mode display
        if (status.mode === 'TELEGRAM') {
            document.getElementById('current-mode').textContent = '📡 TELEGRAM Mode Active';
        } else {
            document.getElementById('current-mode').textContent = '📱 DEFAULT Mode Active';
        }
        
        // Pre-fill form if available
        if (status.wifi_ssid) {
            document.getElementById('wifi-ssid').value = status.wifi_ssid;
        }
        if (status.chat_id) {
            document.getElementById('chat-id').value = status.chat_id;
        }
        
        document.getElementById('mode').value = status.mode || 'DEFAULT';
        
        // Show/hide telegram config based on mode
        const event_change = new Event('change');
        document.getElementById('mode').dispatchEvent(event_change);
    }
});

// Notify Telegram that the app is ready
tg.ready();

