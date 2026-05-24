#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <string.h>

#include "gpio.hpp"
#include "servo.hpp"
#include "mecanum_wheel.hpp"
#include "websocket.hpp"
#include "json_parser.hpp"
#include "motor_driver_enabler.hpp"
#include "mdns.hpp"

int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;

    gpio_enabler g_enabler;
    motor_driver_enabler m_enabler{ pin_output{ 13 } };

    mecanum_wheel mecanum {{
        motor{ pin_output{ 16 }, pin_output{ 21 }, pin_pwm{ 20 }, direction::cw },    // 右上
        motor{ pin_output{ 12 }, pin_output{  1 }, pin_pwm{  7 }, direction::cw },    // 右下
        motor{ pin_output{ 25 }, pin_output{ 24 }, pin_pwm{ 23 }, direction::ccw },   // 左下
        motor{ pin_output{  8 }, pin_output{ 18 }, pin_pwm{ 15 }, direction::cw },    // 左上
    }};
    mecanum.begin();
    
    servo axis1{ pin_servo{ 2 }, deg_to_rad(270), { 500, 2500 } };
    servo axis2{ pin_servo{ 3 }, deg_to_rad(270), { 500, 2500 } };
    servo axis3{ pin_servo{ 4 }, deg_to_rad(270), { 500, 2500 } };
    servo axis4{ pin_servo{ 5 }, deg_to_rad(270), { 500, 2500 } };
    servo axis5{ pin_servo{ 6 }, deg_to_rad(270), { 500, 2500 } };

    const int websocket_server_port = std::atoi(argv[1]);

    websocket_server_start({
        .server_port = websocket_server_port,
        .on_server_start = [&](bool is_start_success) {
            if (is_start_success) {
                std::cout << "[ OK ] WebSocket server activation: ws://" << get_self_url() << ":" << websocket_server_port << '\n';
            } else {
                std::cerr << "[ NG ] Port unavailable\n";
            }
        },
        .on_open = []() {
            std::cout << "[ OK ] client connected\n";
        },
        .on_close = [&]() {
            mecanum.stop();
            std::cout << "[ OK ] client disconnected\n";
        },
        .on_message = [&](std::string_view message) -> std::string {
            if (const auto receive_data = parse_json(message))
            {
                auto mecanum_power = receive_data->wheel;
                mecanum.move(mecanum_power.x, mecanum_power.y, mecanum_power.turn);

                auto arm_power = receive_data->arm;
                axis1.move(arm_power.axis1);
                axis2.move(arm_power.axis2);
                axis3.move(arm_power.axis3);
                axis4.move(arm_power.axis4);
                axis5.move(arm_power.axis5);
                return "[ OK ]";
            }
            else
            {
                return "[ NG ] Invalid json";
            }
        }
    });
}
