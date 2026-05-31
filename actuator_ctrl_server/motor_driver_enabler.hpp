#pragma once

#include "gpio.hpp"

class motor_driver_enabler
{
    pin_output stby;

public:
    motor_driver_enabler(pin_output&& stdy_pin)
        : stby{ std::move(stdy_pin) }
    {
        stby.write(true);
    }

    ~motor_driver_enabler()
    {
        stby.write(false);
    }
};
