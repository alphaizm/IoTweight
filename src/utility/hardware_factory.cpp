#include "hardware_interface.hpp"
#include "emulator_hardware.hpp"
#include "real_hardware.hpp"

// グローバルインスタンス
static HardwareInterface* g_hardware = nullptr;

HardwareInterface* getHardware()
{
    if (g_hardware == nullptr) {
#if defined(ARDUINO) && defined(ESP_PLATFORM)
        // 実機環境
        g_hardware = new RealHardware();
#else
        // エミュレーター環境
        g_hardware = new EmulatorHardware();
#endif
        g_hardware->begin();
    }
    return g_hardware;
}
