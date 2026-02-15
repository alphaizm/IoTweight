#ifndef __WIFI_WEBSERVER_HPP__
#define __WIFI_WEBSERVER_HPP__

#if defined(ARDUINO) && defined(ESP_PLATFORM)
#include <WebServer.h>
#include <DNSServer.h>
#endif

/**
 * @brief WiFi設定用Webサーバー
 * APモード時にWebインターフェースを提供
 */
class WiFiWebServer {
public:
    WiFiWebServer();
    ~WiFiWebServer();
    
    /**
     * @brief Webサーバーを開始
     * @param port ポート番号（デフォルト: 80）
     */
    void begin(int port = 80);
    
    /**
     * @brief Webサーバーを停止
     */
    void stop();
    
    /**
     * @brief Webサーバーのリクエスト処理（ループ内で呼び出す）
     */
    void handleClient();
    
    /**
     * @brief WiFi設定が完了したかチェック
     */
    bool isConfigured() const { return configured; }
    
    /**
     * @brief 設定されたSSIDを取得
     */
    const char* getSSID() const { return ssid; }
    
    /**
     * @brief 設定されたパスワードを取得
     */
    const char* getPassword() const { return password; }
    
    /**
     * @brief 設定をクリア
     */
    void clearConfig();

private:
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    WebServer* server;
    DNSServer* dnsServer;
#endif
    bool configured;
    char ssid[64];
    char password[64];
    
    void handleRoot();
    void handleConfig();
    void handleScan();
    
    static const char* getIndexHTML();
};

#endif  // __WIFI_WEBSERVER_HPP__
