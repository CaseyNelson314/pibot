#pragma once

#include <array>
#include <iostream>
#include <algorithm>

#include "motor.hpp"
#include "type.hpp"

class mecanum_wheel
{
    std::array<motor, 4> wheels;

public:
    mecanum_wheel(std::array<motor, 4>&& wheels)
        : wheels{ std::move(wheels) }
    {
    }

    ~mecanum_wheel()
    {
    }

    void begin()
    {
        for (auto& wheel : wheels)
        {
            wheel.begin();
        }
    }

    /// @brief
    /// @param x -1~1
    /// @param y -1~1
    /// @param turn -1~1
    void move(float x, float y, float turn)
    {
        std::array<float, 4> powers{ {
            +x + -y + turn,
            -x + -y + turn,
            -x + +y + turn,
            +x + +y + turn,
        } };

        // 上で算出した出力値は {powerLimit} を超える場合があります。
        // 超えた場合、最大出力のモジュールの出力を {powerLimit} として他のモジュールの出力を圧縮します。
        auto max = std::abs(*std::max_element(powers.begin(), powers.end(), [](double lhs, double rhs)
                                              { return std::abs(lhs) < std::abs(rhs); }));

        if (max > 1)
        {
            const auto compress_ratio = 1 / max;
            for (float& power : powers)
            {
                power *= compress_ratio;
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            std::cout << i << " : " << powers[i] * 255 << std::endl;
            wheels[i].move(powers[i] * 255);
        }
    }

    void stop()
    {
        for (auto& wheel : wheels)
        {
            wheel.stop();
        }
    }
};
