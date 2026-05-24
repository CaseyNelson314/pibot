#pragma once

#include <cmath>
#include "range.hpp"

using pin_servo = int;

class servo {
    pin_servo pin;
    float angle_limit_rad;
    range<int> pulse_range;
    int last_pulse_us = -1;   // 追加: 前回送ったパルス幅
public:
    servo(pin_servo pin, float angle_limit_rad = M_PI, range<int>&& pulse_range_us = { 500, 2500 });
    void move(float angle_rad);
};