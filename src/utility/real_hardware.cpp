#include "real_hardware.hpp"

#if defined(ARDUINO) && defined(ESP_PLATFORM)
#include <Wire.h>

// MPU6886 (IMU) registers
#define MPU6886_ADDRESS     0x68
#define MPU6886_WHOAMI      0x75
#define MPU6886_ACCEL_XOUT_H 0x3B
#define MPU6886_GYRO_XOUT_H  0x43
#define MPU6886_PWR_MGMT_1   0x6B
#define MPU6886_ACCEL_CONFIG 0x1C
#define MPU6886_GYRO_CONFIG  0x1B

// AXP192 (Power management) registers
#define AXP192_ADDRESS      0x34
#define AXP192_BATT_VOLTAGE 0x78
#define AXP192_BATT_POWER   0x70

RealHardware::RealHardware()
    : current_brightness(128)
    , wifi_status(WiFiStatus::DISCONNECTED)
    , wifi_connect_start(0)
{
}

RealHardware::~RealHardware()
{
}

void RealHardware::begin()
{
    Serial.println("Hardware init: Skipping M5.begin() to preserve display");
    
    // I2C initialization
    Wire.begin(21, 22);
    Wire.setClock(400000);
    Serial.println("  I2C initialized (SDA=21, SCL=22)");
    
    // Button initialization (GPIO)
    pinMode(37, INPUT_PULLUP);
    pinMode(39, INPUT_PULLUP);
    Serial.println("  Buttons initialized (GPIO 37, 39)");
    
    // IMU initialization (MPU6886)
    Wire.beginTransmission(MPU6886_ADDRESS);
    Wire.write(MPU6886_PWR_MGMT_1);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(10);
    
    Wire.beginTransmission(MPU6886_ADDRESS);
    Wire.write(MPU6886_ACCEL_CONFIG);
    Wire.write(0x10);
    Wire.endTransmission();
    
    Wire.beginTransmission(MPU6886_ADDRESS);
    Wire.write(MPU6886_GYRO_CONFIG);
    Wire.write(0x18);
    Wire.endTransmission();
    
    // WHO_AM_I check
    Wire.beginTransmission(MPU6886_ADDRESS);
    Wire.write(MPU6886_WHOAMI);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6886_ADDRESS, 1);
    uint8_t whoami = Wire.read();
    
    if (whoami == 0x19) {
        Serial.println("  IMU (MPU6886) initialized successfully");
    } else {
        Serial.printf("  IMU initialization failed! WHO_AM_I=0x%02X\n", whoami);
    }
    
    Serial.println("Hardware init completed WITHOUT M5Unified");
}

void RealHardware::update()
{
}

bool RealHardware::isButtonAPressed()
{
    return digitalRead(37) == LOW;
}

bool RealHardware::isButtonBPressed()
{
    return digitalRead(39) == LOW;
}

bool RealHardware::wasButtonAPressed()
{
    static bool last_state = false;
    bool current_state = isButtonAPressed();
    bool was_pressed = (current_state && !last_state);
    last_state = current_state;
    return was_pressed;
}

bool RealHardware::wasButtonBPressed()
{
    static bool last_state = false;
    bool current_state = isButtonBPressed();
    bool was_pressed = (current_state && !last_state);
    last_state = current_state;
    return was_pressed;
}

void RealHardware::getAccel(float* x, float* y, float* z)
{
    Wire.beginTransmission(MPU6886_ADDRESS);
    Wire.write(MPU6886_ACCEL_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6886_ADDRESS, 6);
    
    int16_t ax = (Wire.read() << 8) | Wire.read();
    int16_t ay = (Wire.read() << 8) | Wire.read();
    int16_t az = (Wire.read() << 8) | Wire.read();
    
    *x = ax / 4096.0f;
    *y = ay / 4096.0f;
    *z = az / 4096.0f;
}

void RealHardware::getGyro(float* x, float* y, float* z)
{
    Wire.beginTransmission(MPU6886_ADDRESS);
    Wire.write(MPU6886_GYRO_XOUT_H);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU6886_ADDRESS, 6);
    
    int16_t gx = (Wire.read() << 8) | Wire.read();
    int16_t gy = (Wire.read() << 8) | Wire.read();
    int16_t gz = (Wire.read() << 8) | Wire.read();
    
    *x = gx / 16.4f;
    *y = gy / 16.4f;
    *z = gz / 16.4f;
}

float RealHardware::getBatteryVoltage()
{
    Wire.beginTransmission(AXP192_ADDRESS);
    Wire.write(AXP192_BATT_VOLTAGE);
    Wire.endTransmission(false);
    Wire.requestFrom(AXP192_ADDRESS, 2);
    
    if (Wire.available() >= 2) {
        uint16_t raw = (Wire.read() << 4) | Wire.read();
        return raw * 1.1f / 1000.0f;
    }
    
    return 0.0f;
}

int RealHardware::getBatteryLevel()
{
    float voltage = getBatteryVoltage();
    
    if (voltage >= 4.1f) return 100;
    if (voltage >= 3.95f) return 75 + (int)((voltage - 3.95f) / 0.15f * 25);
    if (voltage >= 3.73f) return 25 + (int)((voltage - 3.73f) / 0.22f * 50);
    if (voltage >= 3.3f) return (int)((voltage - 3.3f) / 0.43f * 25);
    return 0;
}

void RealHardware::setBrightness(uint8_t brightness)
{
    current_brightness = brightness;
}

uint8_t RealHardware::getBrightness()
{
    return current_brightness;
}

// ====================================================================
// WiFi機能実装（実機用）
// ====================================================================

bool RealHardware::hasWiFiConfig()
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    preferences.begin("wifi", true);
    bool configured = preferences.getBool("configured", false);
    preferences.end();
    return configured;
#else
    return false;
#endif
}

bool RealHardware::loadWiFiConfig(String& ssid, String& password)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    preferences.begin("wifi", true);
    
    if (!preferences.getBool("configured", false)) {
        preferences.end();
        return false;
    }
    
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    preferences.end();
    
    if (ssid.length() == 0) {
        return false;
    }
    
    return true;
#else
    return false;
#endif
}

void RealHardware::saveWiFiConfig(const String& ssid, const String& password)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putBool("configured", true);
    preferences.end();
    
    Serial.println("[WiFi] Configuration saved");
#endif
}

void RealHardware::clearWiFiConfig()
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    preferences.begin("wifi", false);
    preferences.clear();
    preferences.end();
    
    Serial.println("[WiFi] Configuration cleared");
#endif
}

WiFiStatus RealHardware::getWiFiStatus()
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    // 接続中タイムアウトチェック（10秒）
    if (wifi_status == WiFiStatus::CONNECTING) {
        if (millis() - wifi_connect_start > 10000) {
            wifi_status = WiFiStatus::FAILED;
        } else if (WiFi.status() == WL_CONNECTED) {
            wifi_status = WiFiStatus::CONNECTED;
        }
    }
#endif
    return wifi_status;
}

bool RealHardware::connectWiFi(const String& ssid, const String& password)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.printf("[WiFi] Connecting to '%s'...\n", ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    wifi_status = WiFiStatus::CONNECTING;
    wifi_connect_start = millis();
    
    return true;
#else
    return false;
#endif
}

void RealHardware::disconnectWiFi()
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    WiFi.disconnect();
    wifi_status = WiFiStatus::DISCONNECTED;
    Serial.println("[WiFi] Disconnected");
#endif
}

int RealHardware::scanNetworks(WiFiNetwork* networks, int maxNetworks)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("[WiFi] Scanning networks...");
    
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        Serial.println("[WiFi] No networks found");
        return 0;
    }
    
    int count = (n < maxNetworks) ? n : maxNetworks;
    
    for (int i = 0; i < count; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    }
    
    Serial.printf("[WiFi] Found %d networks\n", count);
    return count;
#else
    return 0;
#endif
}

bool RealHardware::startAPMode(const String& ssid)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.printf("[WiFi] Starting AP mode: %s\n", ssid.c_str());
    
    // APモード設定
    WiFi.mode(WIFI_AP);
    
    // IPアドレスを明示的に設定
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("[WiFi] AP config failed!");
        return false;
    }
    
    // APを起動
    bool success = WiFi.softAP(ssid.c_str());
    
    if (success) {
        wifi_status = WiFiStatus::AP_MODE;
        IPAddress IP = WiFi.softAPIP();
        Serial.print("[WiFi] AP IP address: ");
        Serial.println(IP);
        Serial.printf("[WiFi] AP SSID: %s (no password)\n", ssid.c_str());
        Serial.println("[WiFi] AP started successfully - ready for connections");
        
        // 接続されているクライアント数を定期的に確認するためのログ
        Serial.println("[WiFi] Waiting for clients to connect...");
    } else {
        Serial.println("[WiFi] Failed to start AP!");
    }
    
    return success;
#else
    return false;
#endif
}

void RealHardware::stopAPMode()
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    WiFi.softAPdisconnect(true);
    wifi_status = WiFiStatus::DISCONNECTED;
    Serial.println("[WiFi] AP mode stopped");
#endif
}

String RealHardware::getIPAddress()
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    if (wifi_status == WiFiStatus::CONNECTED) {
        return WiFi.localIP().toString();
    } else if (wifi_status == WiFiStatus::AP_MODE) {
        return WiFi.softAPIP().toString();
    } else {
        return "0.0.0.0";
    }
#else
    return "0.0.0.0";
#endif
}

#endif
