#pragma once

#include <cmath>

inline float deg_to_rad(int deg)
{
    return (2 * M_PI) * deg / 360;
}

inline float clamp(float value, float min_value, float max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

struct vec2
{
    float x, y;
};

struct xyturn
{
    vec2 xy;
    float turn;
};

enum class direction
{
    cw,
    ccw,
};

inline int direction_to_sign(direction dir)
{
    switch (dir)
    {
    case direction::cw: return 1;
    case direction::ccw: return -1;
    default: return 0;  // unreachable
    }
}
