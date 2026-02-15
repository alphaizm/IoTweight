#include "qrcode_generator.hpp"
#include <string.h>

// qrcode.cライブラリを使用（PlatformIOでインストール必要）
#if defined(ARDUINO) && defined(ESP_PLATFORM)
#include "qrcode.h"
#endif

#include "lvgl.h"

bool QRCodeGenerator::generate(const char* text, uint8_t& outSize, uint8_t qrcode[MAX_SIZE][MAX_SIZE])
{
#if defined(ARDUINO) && defined(ESP_PLATFORM)
    // qrcode.cライブラリを使用
    QRCode qr;
    uint8_t qrcodeBytes[qrcode_getBufferSize(3)];
    
    int result = qrcode_initText(&qr, qrcodeBytes, 3, ECC_LOW, text);
    
    if (result != 0) {
        return false;
    }
    
    outSize = qr.size;
    
    // データをコピー
    for (uint8_t y = 0; y < qr.size; y++) {
        for (uint8_t x = 0; x < qr.size; x++) {
            qrcode[y][x] = qrcode_getModule(&qr, x, y) ? 1 : 0;
        }
    }
    
    return true;
#else
    // エミュレーター環境：ダミーQRコード（市松模様）
    outSize = 21;  // QRコードバージョン1のサイズ
    
    for (uint8_t y = 0; y < outSize; y++) {
        for (uint8_t x = 0; x < outSize; x++) {
            // 市松模様パターン
            qrcode[y][x] = ((x + y) % 2 == 0) ? 1 : 0;
        }
    }
    
    // 位置検出パターン風の模様を追加（左上、右上、左下）
    for (int i = 0; i < 7; i++) {
        for (int j = 0; j < 7; j++) {
            bool isEdge = (i == 0 || i == 6 || j == 0 || j == 6);
            bool isCenter = (i >= 2 && i <= 4 && j >= 2 && j <= 4);
            uint8_t val = (isEdge || isCenter) ? 1 : 0;
            
            qrcode[i][j] = val;  // 左上
            qrcode[i][outSize - 1 - j] = val;  // 右上
            qrcode[outSize - 1 - i][j] = val;  // 左下
        }
    }
    
    return true;
#endif
}

void QRCodeGenerator::drawToCanvas(void* canvas, uint8_t qrcode[MAX_SIZE][MAX_SIZE], uint8_t size, uint8_t scale)
{
    lv_obj_t* canvasObj = (lv_obj_t*)canvas;
    
    for (uint8_t y = 0; y < size; y++) {
        for (uint8_t x = 0; x < size; x++) {
            lv_color_t color = qrcode[y][x] ? lv_color_black() : lv_color_white();
            
            // スケール分のピクセルを塗りつぶす
            for (uint8_t sy = 0; sy < scale; sy++) {
                for (uint8_t sx = 0; sx < scale; sx++) {
                    lv_canvas_set_px(canvasObj, x * scale + sx, y * scale + sy, color, LV_OPA_COVER);
                }
            }
        }
    }
}
