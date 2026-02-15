#include "lvgl.h"
#include "lvgl_port_m5stack.hpp"
#include "hardware_interface.hpp"
#include "qrcode_generator.hpp"
#include "wifi_webserver.hpp"
#include <stdio.h>
#include <string.h>

#if defined(ARDUINO) && defined(ESP_PLATFORM)
#include <ESP.h>
#endif

// 画面状態管理
enum AppScreen {
    SCREEN_WIFI_SETUP,  // WiFi設定画面
    SCREEN_START,       // スタート画面
    SCREEN_MAIN,        // メイン画面（ハードウェアデモ）
    SCREEN_VERSION      // バージョン表示画面
};

static AppScreen current_screen = SCREEN_START;

// WiFi設定画面用の変数
static lv_obj_t* label_wifi_status = nullptr;
static lv_obj_t* label_wifi_ssid = nullptr;
static lv_obj_t* label_wifi_ip = nullptr;
static lv_obj_t* qrcode_canvas = nullptr;
static WiFiWebServer* webServer = nullptr;
static uint8_t qrcode_data[QRCodeGenerator::MAX_SIZE][QRCodeGenerator::MAX_SIZE];
static uint8_t qrcode_size = 2;

// ハードウェアインタラクティブなデモアプリ
static lv_obj_t* label_status = nullptr;
static lv_obj_t* label_accel = nullptr;
static lv_obj_t* label_battery = nullptr;

#ifndef APP_VERSION
#define APP_VERSION "0.0.1"
#endif

static void format_build_datetime(char* out, int out_size)
{
    const char* date = __DATE__;  // "Mmm dd yyyy"
    const char* time = __TIME__;  // "hh:mm:ss"

    static const char* months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    int month = 1;
    for (int i = 0; i < 12; i++) {
        if (0 == strncmp(date, months[i], 3)) {
            month = i + 1;
            break;
        }
    }

    int day = (' ' == date[4]) ? (date[5] - '0') : ((date[4] - '0') * 10 + (date[5] - '0'));
    int year = (date[7] - '0') * 1000 + (date[8] - '0') * 100 + (date[9] - '0') * 10 + (date[10] - '0');

    // HH:MM のみ使用
    snprintf(out, out_size, "%04d/%02d/%02d %.5s", year, month, day, time);
}

///////////////////////////////////////////////////////////
//      内部関数
///////////////////////////////////////////////////////////

///////////////////////////////////////
/// @brief WiFi設定画面のUIを作成（APモード + QRコード表示）
void create_screen_wifi_setup(void)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Creating WiFi setup screen (AP Mode + QR Code)...");
#else
    printf("Creating WiFi setup screen (AP Mode + QR Code)...\n");
#endif
    
    HardwareInterface* hw = getHardware();
    
    // APモードで起動
    String ap_ssid;
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    uint8_t mac[6];
    WiFi.macAddress(mac);
    ap_ssid = String("M5Stick-") + String(mac[4], HEX) + String(mac[5], HEX);
    ap_ssid.toUpperCase();
#else
    ap_ssid = "M5Stick-F1C8";
#endif
    
    hw->startAPMode(ap_ssid);
    
    // IPアドレス取得
    String ip = hw->getIPAddress();
    
    // Webサーバー起動
    webServer = new WiFiWebServer();
    webServer->begin(80);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.printf("AP Mode: SSID=%s, IP=%s\n", ap_ssid.c_str(), ip.c_str());
#else
    printf("AP Mode: SSID=%s, IP=%s\n", ap_ssid.c_str(), ip.c_str());
#endif
    
    // QRコード生成（設定URLを含む）
    String qr_text = "http://" + ip + "/";
    QRCodeGenerator::generate(qr_text.c_str(), qrcode_size, qrcode_data);
    
    // UI作成
    lv_obj_t* scr = lv_scr_act();
    
    // 背景を黒に設定
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // タイトル（白文字）
    lv_obj_t* label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "WiFi Setup");
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 2);
    
    // SSID表示（緑文字）
    label_wifi_ssid = lv_label_create(scr);
    lv_label_set_text_fmt(label_wifi_ssid, "SSID: %s", ap_ssid.c_str());
    lv_obj_set_style_text_color(label_wifi_ssid, lv_color_make(0, 255, 0), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_ssid, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(label_wifi_ssid, LV_ALIGN_TOP_LEFT, 5, 22);
    
    // QRコードキャンバス（右側）
    int qr_scale = 2;  // 各モジュールのピクセル数
    int canvas_size = qrcode_size * qr_scale + 8;  // マージン含む
    
    static lv_color_t cbuf[80 * 80];  // キャンバスバッファ
    qrcode_canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(qrcode_canvas, cbuf, canvas_size, canvas_size, LV_COLOR_FORMAT_RGB565);
    lv_canvas_fill_bg(qrcode_canvas, lv_color_white(), LV_OPA_COVER);
    lv_obj_align(qrcode_canvas, LV_ALIGN_TOP_RIGHT, -30, 50);
    
    // QRコード描画（中央に配置）
    int offset = 4;
    for (uint8_t y = 0; y < qrcode_size; y++) {
        for (uint8_t x = 0; x < qrcode_size; x++) {
            lv_color_t color = qrcode_data[y][x] ? lv_color_black() : lv_color_white();
            for (int sy = 0; sy < qr_scale; sy++) {
                for (int sx = 0; sx < qr_scale; sx++) {
                    lv_canvas_set_px(qrcode_canvas, offset + x * qr_scale + sx, 
                                     offset + y * qr_scale + sy, color, LV_OPA_COVER);
                }
            }
        }
    }
    
    // ステータス表示（黄色文字）
    label_wifi_status = lv_label_create(scr);
    lv_label_set_text(label_wifi_status, "Scan QR code\nwith smartphone");
    lv_obj_set_style_text_color(label_wifi_status, lv_color_make(255, 255, 0), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_status, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_wifi_status, LV_ALIGN_BOTTOM_LEFT, 5, -20);
    
    // IPアドレス表示（シアン文字）
    label_wifi_ip = lv_label_create(scr);
    lv_label_set_text_fmt(label_wifi_ip, "http://%s/", ip.c_str());
    lv_obj_set_style_text_color(label_wifi_ip, lv_color_make(0, 255, 255), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_wifi_ip, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_align(label_wifi_ip, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("WiFi setup screen created with QR code");
#else
    printf("WiFi setup screen created with QR code\n");
#endif
}

///////////////////////////////////////
/// @brief スタート画面のUIを作成
void create_screen_start(void)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Creating start screen UI...");
    delay(100);
#else
    printf("Creating start screen UI...\n");
#endif
    
    // スクリーンオブジェクトを取得
    lv_obj_t* scr = lv_scr_act();
    
    // 背景を黒に設定
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
    // タイトル（白文字・大きめ）
    lv_obj_t* label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "IoT Weight\nMonitor");
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label_title, LV_ALIGN_CENTER, 0, -20);
    
    // スタート指示（緑文字）
    lv_obj_t* label_instruction = lv_label_create(scr);
    lv_label_set_text(label_instruction, "Press A to start");
    lv_obj_set_style_text_color(label_instruction, lv_color_make(0, 255, 0), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_instruction, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_align(label_instruction, LV_ALIGN_CENTER, 0, 30);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Start screen created successfully");
    delay(100);
#else
    printf("Start screen created successfully\n");
#endif
}

///////////////////////////////////////
/// @brief メイン画面のUIを作成
/// ボタン、加速度、バッテリー情報を表示
void create_screen_main(void)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Creating hardware demo UI...");
    delay(100);
#else
    printf("Creating hardware demo UI...\n");
#endif
    
    // スクリーンオブジェクトを取得
    lv_obj_t* scr = lv_scr_act();
    
    // 背景を黒に設定
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Background set to BLACK");
    delay(100);
#endif
    
    // タイトル（白文字）
    lv_obj_t* label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "M5StickC Plus2");
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 5);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Title created");
    delay(100);
#endif
    
    // ステータス表示（緑文字）
    label_status = lv_label_create(scr);
    lv_label_set_text(label_status, "Press A or B");
    lv_obj_set_style_text_color(label_status, lv_color_make(0, 255, 0), LV_PART_MAIN);
    lv_obj_align(label_status, LV_ALIGN_TOP_LEFT, 5, 25);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Status label created");
    delay(100);
#endif
    
    // 加速度表示（黄色文字）
    label_accel = lv_label_create(scr);
    lv_label_set_text(label_accel, "Accel:\n  - X: 0.0\n  - Y: 0.0\n  - Z: 1.0");
    lv_obj_set_style_text_color(label_accel, lv_color_make(255, 255, 0), LV_PART_MAIN);
    lv_obj_align(label_accel, LV_ALIGN_TOP_LEFT, 5, 50);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Accel label created");
    delay(100);
#endif
    
    // バッテリー表示（シアン文字）
    label_battery = lv_label_create(scr);
    lv_label_set_text(label_battery, "Battery: ---%");
    lv_obj_set_style_text_color(label_battery, lv_color_make(0, 255, 255), LV_PART_MAIN);
    lv_obj_align(label_battery, LV_ALIGN_BOTTOM_LEFT, 5, -5);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("UI created successfully");
    delay(100);
#else
    printf("UI created successfully\n");
#endif
}

///////////////////////////////////////
/// @brief バージョン表示画面のUIを作成
void create_screen_version(void)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("Creating version screen UI...");
#else
    printf("Creating version screen UI...\n");
#endif

    lv_obj_t* scr = lv_scr_act();

    // 背景を黒に設定
    lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

    // タイトル
    lv_obj_t* label_title = lv_label_create(scr);
    lv_label_set_text(label_title, "Version");
    lv_obj_set_style_text_color(label_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_align(label_title, LV_ALIGN_TOP_MID, 0, 8);

    // バージョン情報
    lv_obj_t* label_version = lv_label_create(scr);
    char build_datetime[32];
    format_build_datetime(build_datetime, sizeof(build_datetime));
    lv_label_set_text_fmt(label_version, "APP: %s\nBuild: %s", APP_VERSION, build_datetime);
    lv_obj_set_style_text_color(label_version, lv_color_make(0, 255, 255), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_version, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_set_style_text_align(label_version, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align(label_version, LV_ALIGN_TOP_LEFT, 5, 45);

    // 戻り方
    lv_obj_t* label_instruction = lv_label_create(scr);
    lv_label_set_text(label_instruction, "Press A to return");
    lv_obj_set_style_text_color(label_instruction, lv_color_make(0, 255, 0), LV_PART_MAIN);
    lv_obj_set_style_text_font(label_instruction, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_align(label_instruction, LV_ALIGN_BOTTOM_MID, 0, -8);
}

///////////////////////////////////////
/// @brief ハードウェアデモアプリの更新
/// ボタン押下、加速度、バッテリー情報を更新
void update_screen_main(void)
{
    HardwareInterface* hw = getHardware();
    hw->update();
    
    char buf[128];
    static uint32_t counter = 0;
    static uint32_t button_a_press_start = 0;
    static bool button_a_long_press_triggered = false;
    static uint32_t button_b_press_start = 0;
    static bool button_b_long_press_triggered = false;

    // Aボタン長押しチェック（バージョン画面へ遷移）
    if (hw->isButtonAPressed()) {
        if (0 == button_a_press_start) {
            button_a_press_start = lv_tick_get();
            button_a_long_press_triggered = false;
        } else {
            uint32_t press_duration = lv_tick_get() - button_a_press_start;
            if (1500 < press_duration && !button_a_long_press_triggered) {
                button_a_long_press_triggered = true;

#if defined(ARDUINO) && defined(ESP_PLATFORM)
                Serial.println("Button A long press detected - transitioning to version screen");
#else
                printf("Button A long press detected - transitioning to version screen\n");
#endif

                lv_obj_clean(lv_scr_act());
                create_screen_version();
                current_screen = SCREEN_VERSION;

                button_a_press_start = 0;
                return;
            }
        }
    } else {
        // ボタンが離された
        if (0 != button_a_press_start && !button_a_long_press_triggered) {
            // 短押しの処理
            lv_label_set_text(label_status, "Button A pressed!");
#if defined(ARDUINO) && defined(ESP_PLATFORM)
            Serial.println("Button A pressed!");
#else
            printf("Button A pressed!\n");
#endif
        }
        button_a_press_start = 0;
        button_a_long_press_triggered = false;
    }
    
    // Bボタン長押しチェック（WiFi設定リセット）
    if (hw->isButtonBPressed()) {
        if (0 == button_b_press_start) {
            // 押され始めた時刻を記録
            button_b_press_start = lv_tick_get();
            button_b_long_press_triggered = false;
        } else {
            // 長押し判定（3秒以上）
            uint32_t press_duration = lv_tick_get() - button_b_press_start;
            if (3000 < press_duration && !button_b_long_press_triggered) {
                button_b_long_press_triggered = true;
                
                lv_label_set_text(label_status, "WiFi Reset!\nRebooting...");
#if defined(ARDUINO) && defined(ESP_PLATFORM)
                Serial.println("Button B long press detected - resetting WiFi config");
#else
                printf("Button B long press detected - resetting WiFi config\n");
#endif
                
                // WiFi設定をクリア
                hw->clearWiFiConfig();
                
                // リブート
#if defined(ARDUINO) && defined(ESP_PLATFORM)
                delay(1000);
                ESP.restart();
#else
                // エミュレーターでは画面遷移のみ
                printf("Simulating reboot to WiFi setup screen...\n");
                lv_obj_clean(lv_scr_act());
                create_screen_wifi_setup();
                current_screen = SCREEN_WIFI_SETUP;
                button_b_press_start = 0;
                return;
#endif
            } else if (!button_b_long_press_triggered) {
                // 長押し中の視覚的フィードバック
                snprintf(buf, sizeof(buf), "Hold B: %d/3s", (int)(press_duration / 1000) + 1);
                lv_label_set_text(label_status, buf);
            }
        }
    } else {
        // ボタンが離された
        if (0 != button_b_press_start && !button_b_long_press_triggered) {
            // 短押しの処理（明るさ変更）
            lv_label_set_text(label_status, "Button B pressed!");
#if defined(ARDUINO) && defined(ESP_PLATFORM)
            Serial.println("Button B pressed!");
            
            extern M5GFX gfx;
            static uint8_t brightness = 128;
            brightness += 64;
            gfx.setBrightness(brightness);
            Serial.printf("Brightness set to %d\n", brightness);
#else
            printf("Button B pressed!\n");
            static uint8_t brightness = 128;
            brightness += 64;
            hw->setBrightness(brightness);
#endif
        }
        button_b_press_start = 0;
        button_b_long_press_triggered = false;
    }
    
    // 加速度表示（10回に1回更新）
    if (0 == counter % 10) {
        float ax, ay, az;
        hw->getAccel(&ax, &ay, &az);
        snprintf(buf, sizeof(buf), "Accel:\n  - X: %.2f\n  - Y: %.2f\n  - Z: %.2f", ax, ay, az);
        lv_label_set_text(label_accel, buf);
#if defined(ARDUINO) && defined(ESP_PLATFORM)
        if (0 == counter % 100) {  // 100回に1回シリアル出力
            Serial.printf("Accel: X=%.2f Y=%.2f Z=%.2f\n", ax, ay, az);
        }
#endif
    }
    
    // バッテリー表示（100回に1回更新）
    if (0 == counter % 100) {
        int battery = hw->getBatteryLevel();
        float voltage = hw->getBatteryVoltage();
        snprintf(buf, sizeof(buf), "Battery: %d%% %.2fV", battery, voltage);
        lv_label_set_text(label_battery, buf);
#if defined(ARDUINO) && defined(ESP_PLATFORM)
        Serial.printf("Battery: %d%% %.2fV\n", battery, voltage);
#endif
    }
    
    counter++;
}

///////////////////////////////////////
/// @brief WiFi設定画面の更新（Webサーバー処理）
void update_wifi_setup(void)
{
    HardwareInterface* hw = getHardware();
    
    // Webサーバーのリクエスト処理（LVGLロック不要）
    if (webServer) {
        webServer->handleClient();
        
        // WiFi設定が完了したかチェック
        if (webServer->isConfigured()) {
            const char* ssid = webServer->getSSID();
            const char* password = webServer->getPassword();
            
#if defined(ARDUINO) && defined(ESP_PLATFORM)
            Serial.printf("WiFi configuration received: %s\n", ssid);
#else
            printf("WiFi configuration received: %s\n", ssid);
#endif
            
            // 設定を保存
            hw->saveWiFiConfig(ssid, password);
            
            // ステータス更新（LVGLロックが必要）
            if (lvgl_port_lock()) {
                lv_label_set_text(label_wifi_status, "Config saved!\nRebooting...");
                lvgl_port_unlock();
            }
            
            // Webサーバー停止
            webServer->stop();
            delete webServer;
            webServer = nullptr;
            
            // APモード停止
            hw->stopAPMode();
            
            // リブート
#if defined(ARDUINO) && defined(ESP_PLATFORM)
            delay(2000);
            ESP.restart();
#else
            // エミュレーターでは画面遷移のみ
            printf("Simulating reboot...\n");
            if (lvgl_port_lock()) {
                lv_obj_clean(lv_scr_act());
                create_screen_start();
                current_screen = SCREEN_START;
                lvgl_port_unlock();
            }
            
            // WiFi接続を試行
            hw->connectWiFi(ssid, password);
#endif
        }
    }
    
    // 定期的にステータス更新（点滅効果）- LVGLロックが必要
    static uint32_t last_blink = 0;
    static bool blink_state = false;
    
    if (1000 < lv_tick_get() - last_blink) {
        last_blink = lv_tick_get();
        blink_state = !blink_state;
        
        if (lvgl_port_lock()) {
            if (blink_state) {
                lv_label_set_text(label_wifi_status, "► Scan QR code\nwith smartphone");
            } else {
                lv_label_set_text(label_wifi_status, "  Scan QR code\nwith smartphone");
            }
            lvgl_port_unlock();
        }
    }
}

///////////////////////////////////////////////////////////
//      外部関数
///////////////////////////////////////////////////////////

///////////////////////////////////////
/// @brief ユーザーアプリケーションの初期化
/// WiFi設定をチェックしてから適切な画面を表示
void user_app_setup(void)
{
    HardwareInterface* hw = getHardware();
    
    // WiFi設定の有無をチェック
    if (hw->hasWiFiConfig()) {
        // WiFi設定がある場合、自動接続を試みる
        String ssid;
        String password;
        
        if (hw->loadWiFiConfig(ssid, password)) {
#if defined(ARDUINO) && defined(ESP_PLATFORM)
            Serial.printf("WiFi config found: %s\n", ssid.c_str());
            Serial.println("Attempting to connect...");
#else
            printf("WiFi config found: %s\n", ssid.c_str());
            printf("Attempting to connect...\n");
#endif
            
            hw->connectWiFi(ssid, password);
            
            // 接続結果を待つ（最大5秒）
            for (int cnt = 0; cnt < 50; cnt++) {
                hw->update();
                if (hw->getWiFiStatus() == WiFiStatus::CONNECTED) {
                    String ip = hw->getIPAddress();
#if defined(ARDUINO) && defined(ESP_PLATFORM)
                    Serial.printf("WiFi connected! IP: %s\n", ip.c_str());
#else
                    printf("WiFi connected! IP: %s\n", ip.c_str());
#endif
                    break;
                }
#if defined(ARDUINO) && defined(ESP_PLATFORM)
                delay(100);
#else
                // エミュレーターでは即座に接続成功
                break;
#endif
            }
        }
        
        // スタート画面から開始
        if (lvgl_port_lock()) {
            create_screen_start();
            current_screen = SCREEN_START;
            lvgl_port_unlock();
        }
    } else {
        // WiFi設定がない場合、WiFi設定画面を表示
#if defined(ARDUINO) && defined(ESP_PLATFORM)
        Serial.println("No WiFi config found - showing WiFi setup screen");
#else
        printf("No WiFi config found - showing WiFi setup screen\n");
#endif
        
        if (lvgl_port_lock()) {
            create_screen_wifi_setup();
            current_screen = SCREEN_WIFI_SETUP;
            lvgl_port_unlock();
        }
    }
}

///////////////////////////////////////
/// @brief ユーザーアプリケーションのメインループ
/// 画面状態に応じた処理
void user_app_loop(void)
{
    // ハードウェア更新
    HardwareInterface* hw = getHardware();
    hw->update();
    
    // 画面状態に応じた処理
    switch (current_screen) {
        case SCREEN_WIFI_SETUP:
            // WiFi設定画面：Webサーバー処理（LVGLロック不要）
            update_wifi_setup();
            break;
            
        case SCREEN_START:
            // スタート画面：Aボタンでメイン画面へ遷移
            if (lvgl_port_lock()) {
                if (hw->wasButtonAPressed()) {
#if defined(ARDUINO) && defined(ESP_PLATFORM)
                    Serial.println("Button A pressed - transitioning to main screen");
#else
                    printf("Button A pressed - transitioning to main screen\n");
#endif
                    lv_obj_clean(lv_scr_act());  // 現在の画面内容をクリア
                    create_screen_main();
                    current_screen = SCREEN_MAIN;
                }
                lvgl_port_unlock();
            }
            break;
            
        case SCREEN_MAIN:
            // メイン画面：ハードウェアデモの更新
            if (lvgl_port_lock()) {
                update_screen_main();
                lvgl_port_unlock();
            }
            break;

        case SCREEN_VERSION:
            // バージョン画面：Aボタン短押しでメイン画面へ戻る
            if (lvgl_port_lock()) {
                if (hw->wasButtonAPressed()) {
#if defined(ARDUINO) && defined(ESP_PLATFORM)
                    Serial.println("Button A short press - returning to main screen");
#else
                    printf("Button A short press - returning to main screen\n");
#endif
                    lv_obj_clean(lv_scr_act());
                    create_screen_main();
                    current_screen = SCREEN_MAIN;
                }
                lvgl_port_unlock();
            }
            break;
            
        default:
            // 未定義の画面状態（エラー処理）
#if defined(ARDUINO) && defined(ESP_PLATFORM)
            Serial.printf("Unknown screen state: %d\n", current_screen);
#else
            printf("Unknown screen state: %d\n", current_screen);
#endif
            break;
    }
}