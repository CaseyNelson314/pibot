#include <pigpio.h>
#include <cassert>
#include "gpio.hpp"

gpio_enabler::gpio_enabler() noexcept
{
    assert(not(gpioInitialise() < 0));
}

gpio_enabler::~gpio_enabler()
{
    gpioTerminate();
}

pin_output::pin_output(int pin)
    : pin{ pin }
{
}

void pin_output::begin()
{
    gpioSetMode(pin, PI_OUTPUT);
}

void pin_output::write(bool is_high)
{
    gpioWrite(pin, is_high);
}


pin_pwm::pin_pwm(int pin)
    : pin{ pin }
{
}

pin_pwm::~pin_pwm()
{
    gpioSetMode(pin, PI_INPUT);    // 入力モードに戻す これをしないとHIGHが出力され続けた
}

void pin_pwm::begin()
{
    gpioSetPWMfrequency(pin, 100'000);
}

void pin_pwm::write(int angle)
{
    gpioPWM(pin, angle);
}
