#include "emulator_hardware.hpp"

#if !defined(ARDUINO) && (__has_include(<SDL2/SDL.h>) || __has_include(<SDL.h>))
// エミュレーター環境でのみコンパイル

#include <stdio.h>
#include <cmath>
#include <fstream>
#include <string.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#endif

EmulatorHardware::EmulatorHardware()
    : btnA_pressed(false)
    , btnB_pressed(false)
    , btnA_was_pressed(false)
    , btnB_was_pressed(false)
    , accel_x(0.0f)
    , accel_y(0.0f)
    , accel_z(1.0f)
    , gyro_x(0.0f)
    , gyro_y(0.0f)
    , gyro_z(0.0f)
    , weight_grams(0.0f)
    , brightness(128)
    , battery_voltage(4.2f)
    , wifi_status(WiFiStatus::DISCONNECTED)
    , wifi_ip("0.0.0.0")
{
}

EmulatorHardware::~EmulatorHardware()
{
}

void EmulatorHardware::begin()
{
    printf("[Emulator Hardware] Initialized\n");
    printf("  Button A: Press 'A' key\n");
    printf("  Button B: Press 'B' key\n");
}

void EmulatorHardware::update()
{
    // SDLイベントの処理
    const Uint8* keystate = SDL_GetKeyboardState(NULL);
    
    // 前回の状態を保存
    bool btnA_prev = btnA_pressed;
    bool btnB_prev = btnB_pressed;
    
    // 現在の状態を更新
    btnA_pressed = keystate[SDL_SCANCODE_A] != 0;
    btnB_pressed = keystate[SDL_SCANCODE_B] != 0;
    
    // wasPressed検出（立ち上がりエッジ）
    btnA_was_pressed = btnA_pressed && !btnA_prev;
    btnB_was_pressed = btnB_pressed && !btnB_prev;
    
    // モックIMUデータ（簡単なシミュレーション）
    static float angle = 0.0f;
    angle += 0.01f;
    accel_x = std::sin(angle) * 0.1f;
    accel_y = std::cos(angle) * 0.1f;
    accel_z = 1.0f;

    // モック重量データ（0g〜2000gの間で変動）
    weight_grams = 1000.0f + std::sin(angle * 0.35f) * 1000.0f;
    
    // バッテリーレベルを徐々に減少（デモ用）
    static int counter = 0;
    if (++counter > 1000) {
        battery_voltage -= 0.01f;
        if (battery_voltage < 3.0f) battery_voltage = 4.2f;
        counter = 0;
    }
}

bool EmulatorHardware::isButtonAPressed()
{
    return btnA_pressed;
}

bool EmulatorHardware::isButtonBPressed()
{
    return btnB_pressed;
}

bool EmulatorHardware::wasButtonAPressed()
{
    bool result = btnA_was_pressed;
    btnA_was_pressed = false;  // 読み取り後クリア
    return result;
}

bool EmulatorHardware::wasButtonBPressed()
{
    bool result = btnB_was_pressed;
    btnB_was_pressed = false;  // 読み取り後クリア
    return result;
}

void EmulatorHardware::getAccel(float* x, float* y, float* z)
{
    if (x) *x = accel_x;
    if (y) *y = accel_y;
    if (z) *z = accel_z;
}

void EmulatorHardware::getGyro(float* x, float* y, float* z)
{
    if (x) *x = gyro_x;
    if (y) *y = gyro_y;
    if (z) *z = gyro_z;
}

float EmulatorHardware::getBatteryVoltage()
{
    return battery_voltage;
}

int EmulatorHardware::getBatteryLevel()
{
    // 3.0V-4.2Vを0-100%に変換
    float level = (battery_voltage - 3.0f) / (4.2f - 3.0f) * 100.0f;
    if (level < 0.0f) level = 0.0f;
    if (level > 100.0f) level = 100.0f;
    return (int)level;
}

bool EmulatorHardware::hasWeightSensor()
{
    return true;
}

float EmulatorHardware::getWeightGrams()
{
    return weight_grams;
}

bool EmulatorHardware::tareWeightSensor()
{
    printf("[Emulator Weight] Tare done\n");
    return true;
}

bool EmulatorHardware::calibrateWeightSensor(float knownWeightGrams)
{
    printf("[Emulator Weight] Calibrated with %.1fg\n", knownWeightGrams);
    return true;
}

void EmulatorHardware::setBrightness(uint8_t value)
{
    brightness = value;
    printf("[Emulator Hardware] Brightness set to %d\n", brightness);
}

uint8_t EmulatorHardware::getBrightness()
{
    return brightness;
}

// ====================================================================
// WiFi機能実装（エミュレーター用）
// ====================================================================

bool EmulatorHardware::loadWiFiConfigFromFile()
{
    std::ifstream file("wifi_config.txt");
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    bool has_ssid = false;
    bool has_password = false;
    
    while (std::getline(file, line)) {
        if (line.find("SSID=") == 0) {
            wifi_ssid = line.substr(5);
            has_ssid = true;
        } else if (line.find("PASSWORD=") == 0) {
            wifi_password = line.substr(9);
            has_password = true;
        }
    }
    
    file.close();
    return has_ssid && has_password && !wifi_ssid.empty();
}

void EmulatorHardware::saveWiFiConfigToFile()
{
    std::ofstream file("wifi_config.txt");
    if (file.is_open()) {
        file << "SSID=" << wifi_ssid << std::endl;
        file << "PASSWORD=" << wifi_password << std::endl;
        file.close();
        printf("[Emulator WiFi] Configuration saved to wifi_config.txt\n");
    }
}

bool EmulatorHardware::hasWiFiConfig()
{
    return loadWiFiConfigFromFile();
}

bool EmulatorHardware::loadWiFiConfig(String& ssid, String& password)
{
    if (loadWiFiConfigFromFile()) {
        ssid = wifi_ssid;
        password = wifi_password;
        return true;
    }
    return false;
}

void EmulatorHardware::saveWiFiConfig(const String& ssid, const String& password)
{
    wifi_ssid = ssid;
    wifi_password = password;
    saveWiFiConfigToFile();
}

void EmulatorHardware::clearWiFiConfig()
{
    wifi_ssid.clear();
    wifi_password.clear();
    
    // ファイルを削除
    std::remove("wifi_config.txt");
    printf("[Emulator WiFi] Configuration cleared\n");
}

WiFiStatus EmulatorHardware::getWiFiStatus()
{
    return wifi_status;
}

bool EmulatorHardware::connectWiFi(const String& ssid, const String& password)
{
    printf("[Emulator WiFi] Connecting to '%s'...\n", ssid.c_str());
    
    wifi_ssid = ssid;
    wifi_password = password;
    wifi_status = WiFiStatus::CONNECTING;
    
    // エミュレーターでは即座に接続成功とする
    wifi_status = WiFiStatus::CONNECTED;
    wifi_ip = "192.168.1.100";  // モックIPアドレス
    
    printf("[Emulator WiFi] Connected! IP: %s\n", wifi_ip.c_str());
    return true;
}

void EmulatorHardware::disconnectWiFi()
{
    printf("[Emulator WiFi] Disconnected\n");
    wifi_status = WiFiStatus::DISCONNECTED;
    wifi_ip = "0.0.0.0";
}

int EmulatorHardware::scanNetworks(WiFiNetwork* networks, int maxNetworks)
{
    printf("[Emulator WiFi] Scanning networks...\n");
    
    // モックネットワークリストを返す
    const char* mock_ssids[] = {
        "HomeWiFi_2.4G",
        "Office_Network",
        "iPhone_Hotspot",
        "Cafe_Guest"
    };
    int mock_rssi[] = {-45, -62, -71, -85};
    bool mock_encrypted[] = {true, true, true, false};
    
    int count = (maxNetworks < 4) ? maxNetworks : 4;
    
    for (int i = 0; i < count; i++) {
        networks[i].ssid = mock_ssids[i];
        networks[i].rssi = mock_rssi[i];
        networks[i].isEncrypted = mock_encrypted[i];
    }
    
    printf("[Emulator WiFi] Found %d networks\n", count);
    return count;
}

bool EmulatorHardware::startAPMode(const String& ssid)
{
    printf("[Emulator WiFi] Starting AP mode: %s\n", ssid.c_str());
    wifi_status = WiFiStatus::AP_MODE;
    wifi_ssid = ssid;
    wifi_ip = "192.168.4.1";
    return true;
}

void EmulatorHardware::stopAPMode()
{
    printf("[Emulator WiFi] Stopping AP mode\n");
    wifi_status = WiFiStatus::DISCONNECTED;
    wifi_ip = "0.0.0.0";
}

String EmulatorHardware::getIPAddress()
{
    return wifi_ip;
}

#endif  // エミュレーター環境でのみコンパイル
