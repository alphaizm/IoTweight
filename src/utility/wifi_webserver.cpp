#include "wifi_webserver.hpp"
#include <string.h>

// エミュレーター環境用のSDLインクルード
#if !defined(ARDUINO) || !defined(ESP_PLATFORM)
    #if __has_include(<SDL2/SDL.h>)
        #include <SDL2/SDL.h>
    #elif __has_include(<SDL.h>)
        #include <SDL.h>
    #endif
#endif

#if defined(ARDUINO) && defined(ESP_PLATFORM)
#include <WiFi.h>

WiFiWebServer::WiFiWebServer()
    : server(nullptr)
    , dnsServer(nullptr)
    , configured(false)
{
    ssid[0] = '\0';
    password[0] = '\0';
}

WiFiWebServer::~WiFiWebServer()
{
    stop();
}

void WiFiWebServer::begin(int port)
{
    if (server) {
        delete server;
    }
    
    // DNSサーバーを起動（キャプティブポータル用）
    if (!dnsServer) {
        dnsServer = new DNSServer();
    }
    dnsServer->start(53, "*", IPAddress(192, 168, 4, 1));
    Serial.println("[WebServer] DNS Server started (captive portal mode)");
    
    server = new WebServer(port);
    
    // ルートハンドラ
    server->on("/", [this]() {
        Serial.println("[WebServer] Root page requested");
        server->send(200, "text/html", getIndexHTML());
    });
    
    // キャプティブポータル検出用エンドポイント（Android, iOS, Windows）
    server->on("/generate_204", [this]() {
        Serial.println("[WebServer] Captive portal check (Android): /generate_204");
        server->sendHeader("Location", "http://192.168.4.1/", true);
        server->send(302, "text/plain", "");
    });
    
    server->on("/hotspot-detect.html", [this]() {
        Serial.println("[WebServer] Captive portal check (iOS): /hotspot-detect.html");
        server->send(200, "text/html", getIndexHTML());
    });
    
    server->on("/connecttest.txt", [this]() {
        Serial.println("[WebServer] Captive portal check (Windows): /connecttest.txt");
        server->send(200, "text/plain", "Microsoft Connect Test");
    });
    
    server->on("/success.txt", [this]() {
        Serial.println("[WebServer] Captive portal check: /success.txt");
        server->send(200, "text/plain", "success");
    });
    
    // WiFi設定受信ハンドラ
    server->on("/config", [this]() {
        Serial.println("[WebServer] Config endpoint accessed");
        if (server->hasArg("ssid") && server->hasArg("password")) {
            String ssid_str = server->arg("ssid");
            String pass_str = server->arg("password");
            
            strncpy(ssid, ssid_str.c_str(), sizeof(ssid) - 1);
            ssid[sizeof(ssid) - 1] = '\0';
            
            strncpy(password, pass_str.c_str(), sizeof(password) - 1);
            password[sizeof(password) - 1] = '\0';
            
            configured = true;
            
            Serial.printf("[WebServer] WiFi config received: SSID=%s\n", ssid);
            
            server->send(200, "text/html", 
                "<html><body style='font-family:Arial;text-align:center;padding:50px;'>"
                "<h2>Configuration Saved!</h2>"
                "<p>Device will restart and connect to WiFi...</p>"
                "</body></html>");
        } else {
            Serial.println("[WebServer] Missing ssid or password parameter");
            server->send(400, "text/html", "Missing parameters");
        }
    });
    
    // WiFiスキャンハンドラ
    server->on("/scan", [this]() {
        Serial.println("[WebServer] Scan endpoint accessed");
        int n = WiFi.scanNetworks();
        String json = "[";
        
        for (int i = 0; i < n; i++) {
            if (0 < i) json += ",";
            json += "{";
            json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
            json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
            json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
            json += "}";
        }
        
        json += "]";
        server->send(200, "application/json", json);
    });
    
    // 404ハンドラーも追加してデバッグ（すべてのリクエストをルートにリダイレクト）
    server->onNotFound([this]() {
        Serial.printf("[WebServer] Redirecting to root: %s\n", server->uri().c_str());
        server->sendHeader("Location", "http://192.168.4.1/", true);
        server->send(302, "text/plain", "");
    });
    
    server->begin();
    Serial.printf("[WebServer] Web server started on port %d\n", port);
    Serial.println("[WebServer] Waiting for client connections...");
    Serial.println("[WebServer] Registered routes: /, /config, /scan, /generate_204, /hotspot-detect.html, /connecttest.txt, /success.txt");
}

void WiFiWebServer::stop()
{
    if (server) {
        server->stop();
        delete server;
        server = nullptr;
    }
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
}

void WiFiWebServer::handleClient()
{
    if (dnsServer) {
        dnsServer->processNextRequest();
    }
    
    if (server) {
        server->handleClient();
    }
}

void WiFiWebServer::clearConfig()
{
    configured = false;
    ssid[0] = '\0';
    password[0] = '\0';
}

const char* WiFiWebServer::getIndexHTML()
{
    return R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>M5Stick WiFi Setup</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 500px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            background: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            color: #333;
            text-align: center;
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #666;
            font-weight: bold;
        }
        input, select {
            width: 100%;
            padding: 10px;
            border: 1px solid #ddd;
            border-radius: 5px;
            font-size: 16px;
            box-sizing: border-box;
        }
        button {
            width: 100%;
            padding: 12px;
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            margin-top: 10px;
        }
        button:hover {
            background-color: #45a049;
        }
        .scan-btn {
            background-color: #2196F3;
        }
        .scan-btn:hover {
            background-color: #0b7dda;
        }
        .status {
            padding: 10px;
            margin-top: 15px;
            border-radius: 5px;
            text-align: center;
            display: none;
        }
        .status.success {
            background-color: #d4edda;
            color: #155724;
        }
        .status.error {
            background-color: #f8d7da;
            color: #721c24;
        }
        .network-list {
            max-height: 200px;
            overflow-y: auto;
            border: 1px solid #ddd;
            border-radius: 5px;
            padding: 5px;
            display: none;
        }
        .network-item {
            padding: 8px;
            margin: 2px 0;
            background: #f9f9f9;
            border-radius: 3px;
            cursor: pointer;
        }
        .network-item:hover {
            background: #e9e9e9;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔧 M5Stick WiFi Setup</h1>
        
        <div class="form-group">
            <button class="scan-btn" onclick="scanNetworks()">📡 Scan WiFi Networks</button>
        </div>
        
        <div id="networkList" class="network-list"></div>
        
        <form id="wifiForm" onsubmit="submitConfig(event)">
            <div class="form-group">
                <label for="ssid">WiFi SSID:</label>
                <input type="text" id="ssid" name="ssid" required>
            </div>
            
            <div class="form-group">
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" required>
            </div>
            
            <button type="submit">💾 Save & Connect</button>
        </form>
        
        <div id="status" class="status"></div>
    </div>
    
    <script>
        function scanNetworks() {
            document.getElementById('status').style.display = 'none';
            document.getElementById('networkList').innerHTML = '<div style="padding:10px;text-align:center;">Scanning...</div>';
            document.getElementById('networkList').style.display = 'block';
            
            fetch('/scan')
                .then(response => response.json())
                .then(networks => {
                    let html = '';
                    networks.forEach(network => {
                        let lock = network.secure ? '🔒' : '🔓';
                        let strength = network.rssi > -60 ? '📶' : (network.rssi > -70 ? '📶' : '📶');
                        html += `<div class="network-item" onclick="selectNetwork('${network.ssid}')">
                            ${lock} ${network.ssid} ${strength} (${network.rssi} dBm)
                        </div>`;
                    });
                    document.getElementById('networkList').innerHTML = html || '<div style="padding:10px;text-align:center;">No networks found</div>';
                })
                .catch(error => {
                    document.getElementById('networkList').innerHTML = '<div style="padding:10px;text-align:center;color:red;">Scan failed</div>';
                });
        }
        
        function selectNetwork(ssid) {
            document.getElementById('ssid').value = ssid;
            document.getElementById('password').focus();
        }
        
        function submitConfig(event) {
            event.preventDefault();
            
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            
            const status = document.getElementById('status');
            status.textContent = 'Saving configuration...';
            status.className = 'status';
            status.style.display = 'block';
            
            fetch('/config?ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(password))
                .then(response => response.text())
                .then(data => {
                    status.textContent = '✅ Configuration saved! Device will restart...';
                    status.className = 'status success';
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 3000);
                })
                .catch(error => {
                    status.textContent = '❌ Error: ' + error.message;
                    status.className = 'status error';
                });
        }
    </script>
</body>
</html>
)HTML";
}

#else
// エミュレーター環境用の簡易Webサーバー実装

#ifdef _WIN32
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

#include <stdio.h>
#include <string>

WiFiWebServer::WiFiWebServer()
    : configured(false)
{
    ssid[0] = '\0';
    password[0] = '\0';
    
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

WiFiWebServer::~WiFiWebServer()
{
    stop();
#ifdef _WIN32
    WSACleanup();
#endif
}

void WiFiWebServer::begin(int port)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    // Arduino環境では何もしない（上のコードで処理済み）
#else
    printf("[WiFiWebServer] Starting web server on port %d (emulator mode)...\n", port);
    printf("[WiFiWebServer] NOTE: This is a mock implementation.\n");
    printf("[WiFiWebServer] To test WiFi configuration, manually call:\n");
    printf("[WiFiWebServer]   Test SSID: TestSSID, Password: TestPassword\n");
#endif
}

void WiFiWebServer::stop()
{
    printf("[WiFiWebServer] Stopping web server (emulator mode)...\n");
}

void WiFiWebServer::handleClient()
{
    // エミュレーター環境用：キーボード入力でWiFi設定をシミュレート
    // 'W'キーでWiFi設定を受信したことにする
#if !defined(ARDUINO) || !defined(ESP_PLATFORM)
    static bool key_pressed_last = false;
    
    // SDL経由でキー状態を取得
    #if __has_include(<SDL2/SDL.h>) || __has_include(<SDL.h>)
        const Uint8* keystate = SDL_GetKeyboardState(NULL);
        bool key_pressed = keystate[SDL_SCANCODE_W] != 0;
        
        // 'W'キーの立ち上がりエッジを検出
        if (key_pressed && !key_pressed_last) {
            printf("\n[WiFiWebServer] Simulating WiFi configuration received!\n");
            printf("[WiFiWebServer] Press 'W' key detected - auto-configuring...\n");
            
            // テスト用のWiFi設定を保存
            strncpy(ssid, "TestSSID", sizeof(ssid) - 1);
            ssid[sizeof(ssid) - 1] = '\0';
            
            strncpy(password, "TestPassword", sizeof(password) - 1);
            password[sizeof(password) - 1] = '\0';
            
            configured = true;
            
            printf("[WiFiWebServer] Configuration saved: SSID=%s\n", ssid);
        }
        
        key_pressed_last = key_pressed;
    #endif
#endif
}

void WiFiWebServer::clearConfig()
{
    configured = false;
    ssid[0] = '\0';
    password[0] = '\0';
}

const char* WiFiWebServer::getIndexHTML()
{
    return "<!DOCTYPE html><html><body><h1>Emulator Mode</h1><p>Press 'W' key to simulate WiFi config</p></body></html>";
}

#endif
