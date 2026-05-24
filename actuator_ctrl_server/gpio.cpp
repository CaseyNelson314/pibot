#include <lgpio.h>
#include <cassert>
#include "gpio.hpp"

// lgpio はチップを開いて得た handle を各操作に渡す方式。
// pigpio のグローバル初期化と違うため、handle をこの翻訳単位内で共有する。
// (gpio.hpp のインターフェースを変えずに済ませるための内部実装)
namespace
{
    int g_chip_handle = -1;

    // Raspberry Pi 4 以前は gpiochip0、Pi 5 は gpiochip4。
    // まず 0 を試し、失敗したら 4 を試す。
    int open_gpio_chip()
    {
        int h = lgGpiochipOpen(0);
        if (h < 0)
            h = lgGpiochipOpen(4);
        return h;
    }
}

gpio_enabler::gpio_enabler() noexcept
{
    g_chip_handle = open_gpio_chip();
    assert(not(g_chip_handle < 0));
}

gpio_enabler::~gpio_enabler()
{
    if (g_chip_handle >= 0)
    {
        lgGpiochipClose(g_chip_handle);
        g_chip_handle = -1;
    }
}

pin_output::pin_output(int pin)
    : pin{ pin }
{
}

void pin_output::begin()
{
    // 出力として確保 (初期レベル LOW)
    lgGpioClaimOutput(g_chip_handle, 0, pin, 0);
}

void pin_output::write(bool is_high)
{
    lgGpioWrite(g_chip_handle, pin, is_high ? 1 : 0);
}


pin_pwm::pin_pwm(int pin)
    : pin{ pin }
{
}

pin_pwm::~pin_pwm()
{
    // PWM を停止し、GPIO を解放する。
    // pigpio 版では入力モードに戻して HIGH 出力垂れ流しを防いでいた。
    // lgpio では duty 0 で停止 → Free で解放することで同等の状態にする。
    lgTxPwm(g_chip_handle, pin, 1000, 0, 0, 0);
    lgGpioFree(g_chip_handle, pin);
}

void pin_pwm::begin()
{
    // 出力として確保。周波数は write 時に lgTxPwm へ渡す。
    lgGpioClaimOutput(g_chip_handle, 0, pin, 0);
}

void pin_pwm::write(int duty_0_255)
{
    // lgTxPwm の duty は 0〜100 (%)。元コードは 0〜255 を渡すため変換する。
    float duty_percent = (duty_0_255 / 255.0f) * 100.0f;
    if (duty_percent < 0.0f)   duty_percent = 0.0f;
    if (duty_percent > 100.0f) duty_percent = 100.0f;
    lgTxPwm(g_chip_handle, pin, 1000, duty_percent, 0, 0);
}

// servo.cpp から handle を参照するためのアクセサ
int gpio_chip_handle()
{
    return g_chip_handle;
}