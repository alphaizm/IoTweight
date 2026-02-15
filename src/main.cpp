#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lvgl_port_m5stack.hpp"
#include "hardware_interface.hpp"

extern void user_app_setup(void);
extern void user_app_loop(void);
extern void update_screen_main(void);

M5GFX gfx;

void setup(void)
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    // シリアル通信の初期化
    Serial.begin(115200);
    delay(500); // シリアルポートの安定化待ち
    Serial.println("\n\n\n========================================");
    Serial.println("=== M5StickCPlus2 Starting ===");
    Serial.println("========================================");
    Serial.flush();
#endif

    // M5GFXの初期化
    gfx.init();
    gfx.setRotation(1);
    gfx.setBrightness(200);
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("[1] M5GFX initialized");
    
    // ハードウェアの初期化（M5.begin()はスキップ）
    Serial.println("[2] Initializing hardware (buttons only)...");
    HardwareInterface* hw = getHardware();
    hw->begin();
    Serial.flush();
#else
    printf("M5GFX initialized: %dx%d\n", gfx.width(), gfx.height());
#endif

    // LVGLポート初期化
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("[3] Initializing LVGL...");
    Serial.flush();
#endif
    lvgl_port_init(gfx);

    // ユーザーアプリケーション初期化
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("[4] Initializing user app...");
    Serial.flush();
#endif
    user_app_setup();
    
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    Serial.println("[5] Setup complete!");
    Serial.println("========================================");
    Serial.flush();
#endif
}

void loop(void)
{
    // ハードウェアデモの更新
    user_app_loop();

#if defined(ARDUINO) && defined(ESP_PLATFORM)
    delay(10);
#elif !defined(ARDUINO) && (__has_include(<SDL2/SDL.h>) || __has_include(<SDL.h>))
    usleep(10 * 1000);
#endif
}
