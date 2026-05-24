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

// servo.cpp
void servo::move(float angle_rad)
{
    const float angle_pulse = (angle_rad / angle_limit_rad) * pulse_range.diff() + pulse_range.min;
    const int pulse_us = static_cast<int>(angle_pulse);

    if (pulse_us == last_pulse_us)   // 変化なし → 何もしない(保持)
        return;
    last_pulse_us = pulse_us;

    if (std::abs(pulse_us - last_pulse_us) < 3)  // 3μs未満の変化は無視
    return;

    lgTxServo(gpio_chip_handle(), pin, pulse_us, 50, 0, 0);
}