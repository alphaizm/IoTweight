#ifndef __EMULATOR_HARDWARE_HPP__
#define __EMULATOR_HARDWARE_HPP__

#include "hardware_interface.hpp"
#include <string>

/**
 * @brief エミュレーター環境用のハードウェア実装
 * SDLイベントやモックデータを使用
 */
class EmulatorHardware : public HardwareInterface {
public:
    EmulatorHardware();
    ~EmulatorHardware() override;
    
    void begin() override;
    void update() override;
    
    // ボタン関連 (キーボードでシミュレート)
    bool isButtonAPressed() override;
    bool isButtonBPressed() override;
    bool wasButtonAPressed() override;
    bool wasButtonBPressed() override;
    
    // IMU (モックデータ)
    void getAccel(float* x, float* y, float* z) override;
    void getGyro(float* x, float* y, float* z) override;
    
    // バッテリー (モックデータ)
    float getBatteryVoltage() override;
    int getBatteryLevel() override;

    // 重量センサー (モックデータ)
    bool hasWeightSensor() override;
    float getWeightGrams() override;
    bool tareWeightSensor() override;
    bool calibrateWeightSensor(float knownWeightGrams) override;
    
    // LCD輝度
    void setBrightness(uint8_t brightness) override;
    uint8_t getBrightness() override;
    
    // WiFi機能 (ファイルベース設定)
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
    bool btnA_pressed;
    bool btnB_pressed;
    bool btnA_was_pressed;
    bool btnB_was_pressed;
    
    float accel_x, accel_y, accel_z;
    float gyro_x, gyro_y, gyro_z;
    float weight_grams;
    
    uint8_t brightness;
    float battery_voltage;
    
    // WiFi関連
    WiFiStatus wifi_status;
    std::string wifi_ssid;
    std::string wifi_password;
    std::string wifi_ip;
    
    bool loadWiFiConfigFromFile();
    void saveWiFiConfigToFile();
};

#endif  // __EMULATOR_HARDWARE_HPP__
