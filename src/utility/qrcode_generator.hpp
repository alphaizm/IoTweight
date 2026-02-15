#ifndef __QRCODE_GENERATOR_HPP__
#define __QRCODE_GENERATOR_HPP__

#include <stdint.h>

/**
 * @brief シンプルなQRコード生成クラス
 * LVGLで表示するための2次元配列データを生成
 */
class QRCodeGenerator {
public:
    static const int MAX_SIZE = 33;  // QRコードの最大サイズ（バージョン3）
    
    /**
     * @brief QRコードを生成
     * @param text QRコードにエンコードするテキスト
     * @param outSize 生成されたQRコードのサイズ（出力）
     * @param qrcode QRコードデータの出力先（MAX_SIZE x MAX_SIZE配列）
     * @return 成功した場合true
     */
    static bool generate(const char* text, uint8_t& outSize, uint8_t qrcode[MAX_SIZE][MAX_SIZE]);
    
    /**
     * @brief QRコードをLVGLキャンバスに描画
     * @param canvas LVGLキャンバスオブジェクト
     * @param qrcode QRコードデータ
     * @param size QRコードのサイズ
     * @param scale 各モジュールのピクセルサイズ
     */
    static void drawToCanvas(void* canvas, uint8_t qrcode[MAX_SIZE][MAX_SIZE], uint8_t size, uint8_t scale);
};

#endif  // __QRCODE_GENERATOR_HPP__
