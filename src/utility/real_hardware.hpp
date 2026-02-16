#ifndef __REAL_HARDWARE_HPP__
#define __REAL_HARDWARE_HPP__

#include "hardware_interface.hpp"

#if defined(ARDUINO) && defined(ESP_PLATFORM)
#include <M5Unified.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HX711.h>
#endif

/**
 * @brief 実機環境用のハードウェア実装
 * M5Unifiedを使用
 */
class RealHardware : public HardwareInterface {
public:
    RealHardware();
    ~RealHardware() override;
    
    void begin() override;
    void update() override;
    
    // ボタン関連
    bool isButtonAPressed() override;
    bool isButtonBPressed() override;
    bool wasButtonAPressed() override;
    bool wasButtonBPressed() override;
    
    // IMU
    void getAccel(float* x, float* y, float* z) override;
    void getGyro(float* x, float* y, float* z) override;
    
    // バッテリー
    float getBatteryVoltage() override;
    int getBatteryLevel() override;

    // 重量センサー（Scales Kit / HX711）
    bool hasWeightSensor() override;
    float getWeightGrams() override;
    bool tareWeightSensor() override;
    bool calibrateWeightSensor(float knownWeightGrams) override;
    
    // LCD輝度
    void setBrightness(uint8_t brightness) override;
    uint8_t getBrightness() override;
    
    // WiFi機能 (ESP32 Preferences使用)
    bool hasWiFiConfig() override;
    bool loadWiFiConfig(String& ssid, String& password) override;
    void saveWiFiConfig(const String& ssid, const String& password) override;
    void clearWiFiConfig() override;
    WiFiStatus getWiFiStatus() override;
    bool connectWiFi(const String& ssid, const String& password) override;
    void disconnectWiFi() override;
    int scanNetworks(WiFiNetwork* networks, int maxNetworks) override;
    bool startAPMode(const String& ssid) override;
    void stopAPMode() override;
    String getIPAddress() override;

private:
    uint8_t current_brightness;
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Preferences preferences;
    HX711 scale;
#endif
    bool scale_ready;
    WiFiStatus wifi_status;
    unsigned long wifi_connect_start;
};

#endif  // __REAL_HARDWARE_HPP__
