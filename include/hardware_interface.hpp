#ifndef __HARDWARE_INTERFACE_HPP__
#define __HARDWARE_INTERFACE_HPP__

#include <stdint.h>

// Arduino環境ではArduinoのStringを使用、それ以外ではstd::string
#if defined(ARDUINO)
    #include <WString.h>
#else
    #include <string>
    using String = std::string;
#endif

/**
 * @brief WiFi接続状態
 */
enum class WiFiStatus {
    DISCONNECTED,      // 未接続
    CONNECTING,        // 接続中
    CONNECTED,         // 接続済み
    FAILED,           // 接続失敗
    AP_MODE           // APモード（設定モード）
};

/**
 * @brief WiFiネットワーク情報
 */
struct WiFiNetwork {
    String ssid;        // SSID
    int8_t rssi;        // 信号強度 (dBm)
    bool isEncrypted;   // 暗号化の有無
};

/**
 * @brief ハードウェア機能の抽象化インターフェース
 * エミュレーター環境と実機環境の両方で使用可能
 */
class HardwareInterface {
public:
    virtual ~HardwareInterface() {}
    
    // 初期化
    virtual void begin() = 0;
    virtual void update() = 0;
    
    // ボタン関連
    virtual bool isButtonAPressed() = 0;
    virtual bool isButtonBPressed() = 0;
    virtual bool wasButtonAPressed() = 0;
    virtual bool wasButtonBPressed() = 0;
    
    // IMU (加速度センサー)
    virtual void getAccel(float* x, float* y, float* z) = 0;
    virtual void getGyro(float* x, float* y, float* z) = 0;
    
    // バッテリー
    virtual float getBatteryVoltage() = 0;
    virtual int getBatteryLevel() = 0;  // 0-100%
    
    // LCD輝度
    virtual void setBrightness(uint8_t brightness) = 0;  // 0-255
    virtual uint8_t getBrightness() = 0;
    
    // WiFi機能
    virtual bool hasWiFiConfig() = 0;                                    // WiFi設定の有無
    virtual bool loadWiFiConfig(String& ssid, String& password) = 0;    // WiFi設定の読み込み
    virtual void saveWiFiConfig(const String& ssid, const String& password) = 0;  // WiFi設定の保存
    virtual void clearWiFiConfig() = 0;                                 // WiFi設定のクリア
    virtual WiFiStatus getWiFiStatus() = 0;                             // WiFi接続状態の取得
    virtual bool connectWiFi(const String& ssid, const String& password) = 0;  // WiFi接続
    virtual void disconnectWiFi() = 0;                                  // WiFi切断
    virtual int scanNetworks(WiFiNetwork* networks, int maxNetworks) = 0;  // ネットワークスキャン
    virtual bool startAPMode(const String& ssid) = 0;                   // APモード開始
    virtual void stopAPMode() = 0;                                      // APモード停止
    virtual String getIPAddress() = 0;                                  // IPアドレス取得
};

// グローバルインスタンスの取得
HardwareInterface* getHardware();

#endif  // __HARDWARE_INTERFACE_HPP__
