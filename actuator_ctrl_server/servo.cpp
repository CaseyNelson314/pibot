#include <lgpio.h>
#include "servo.hpp"
#include "gpio.hpp"

servo::servo(int pin,
             float angle_limit_rad,
             range<int>&& pulse_range_us)
    : pin{ pin }
    , angle_limit_rad{ angle_limit_rad }
    , pulse_range{ std::move(pulse_range_us) }
{
    // サーボピンを出力として確保
    lgGpioClaimOutput(gpio_chip_handle(), 0, pin, 0);
}

void servo::move(float angle_rad)
{
    const float angle_pulse = (angle_rad / angle_limit_rad) * pulse_range.diff() + pulse_range.min;

    // lgTxServo はパルス幅をマイクロ秒で直接受け取る (pigpio の gpioServo と同じ感覚)。
    // 第4引数 freq=50Hz (一般的なサーボの更新周期), offset=0, cycles=0 (連続出力)
    lgTxServo(gpio_chip_handle(), pin, static_cast<int>(angle_pulse), 50, 0, 0);
}