#include <pigpio.h>
#include "servo.hpp"

servo::servo(int pin,
             float angle_limit_rad,
             range<int>&& pulse_range_us)
    : pin{ pin }
    , angle_limit_rad{ angle_limit_rad }
    , pulse_range{ std::move(pulse_range_us) }
{
}

void servo::move(float angle_rad)
{
    const float angle_pulse = (angle_rad / angle_limit_rad) * pulse_range.diff() + pulse_range.min;
    gpioServo(pin, angle_pulse);
}