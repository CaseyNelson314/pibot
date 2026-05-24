#pragma once


class gpio_enabler
{
public:
    gpio_enabler() noexcept;
    ~gpio_enabler();
    gpio_enabler(const gpio_enabler&) = delete;
    gpio_enabler& operator=(const gpio_enabler&) = delete;
};


class pin_output
{
    int pin;

public:
    pin_output(int pin);
    void begin();
    void write(bool is_high);
};


class pin_pwm
{
    int pin;

public:
    pin_pwm(int pin);
    ~pin_pwm();
    void begin();
    void write(uint8_t angle);
};