#pragma once

#include <optional>
#include <string_view>

struct json_schema
{
    struct wheel_t
    {
        float x;
        float y;
        float turn;
    } wheel;

    struct servo_t_t
    {
        // float axis1;
        // float axis2;
        // float axis3;
        float camera_left_right;
        float camera_up_down;
    } servo;
};

std::optional<json_schema> parse_json(std::string_view json_string);
