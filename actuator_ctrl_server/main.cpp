#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <string.h>
#include <chrono>

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

    const int websocket_server_port = std::atoi(argv[1]);

    gpio_enabler g_enabler;
    motor_driver_enabler m_enabler{ pin_output{ 13 } };

    mecanum_wheel mecanum {{
        motor{ pin_output{ 16 }, pin_output{ 21 }, pin_pwm{ 20 }, direction::cw  },    // 右上
        motor{ pin_output{ 12 }, pin_output{  1 }, pin_pwm{  7 }, direction::cw  },    // 右下
        motor{ pin_output{ 25 }, pin_output{ 24 }, pin_pwm{ 23 }, direction::ccw },    // 左下
        motor{ pin_output{  8 }, pin_output{ 18 }, pin_pwm{ 15 }, direction::cw  },    // 左上
    }};
    mecanum.begin();

    // servo axis1{ pin_servo{ 2 }, deg_to_rad(270), { 500, 2500 } };
    // servo axis2{ pin_servo{ 3 }, deg_to_rad(270), { 500, 2500 } };
    // servo axis3{ pin_servo{ 4 }, deg_to_rad(270), { 500, 2500 } };
    servo camera_left_right{ pin_servo{ 5 }, deg_to_rad(270), { 500, 2500 } };
    servo camera_up_down{ pin_servo{ 6 }, deg_to_rad(270), { 500, 2500 } };



    constexpr int  control_interval_ms = 10;    // モーター制御周期
    constexpr long deadman_timeout_ms  = 500;   // この時間受信が無ければ停止
 
    // 受信した最新の目標値(on_message が書き、on_tick が読む)。
    // uWS の同一イベントループ上で動くため排他制御は不要。
    float target_x = 0.0f;
    float target_y = 0.0f;
    float target_turn = 0.0f;
    auto  last_receive_time = std::chrono::steady_clock::now();
    bool  connected = false;
 
    websocket_server_start({
        .server_port = websocket_server_port,
        .on_server_start = [&](bool is_start_success) {
            if (is_start_success) {
                std::cout << "[ OK ] WebSocket server activation: ws://" << get_self_url() << ":" << websocket_server_port << '\n';
            } else {
                std::cerr << "[ NG ] Port unavailable\n";
            }
        },
        .on_open = [&]() {
            connected = true;
            last_receive_time = std::chrono::steady_clock::now();
            std::cout << "[ OK ] client connected\n";
        },
        .on_close = [&]() {
            connected = false;
            target_x = target_y = target_turn = 0.0f;
            mecanum.stop();
            std::cout << "[ OK ] client disconnected\n";
        },
        .on_message = [&](std::string_view message) -> std::string {
            if (const auto receive_data = parse_json(message))
            {
                target_x = receive_data->wheel.x;
                target_y = receive_data->wheel.y;
                target_turn = receive_data->wheel.turn;
                last_receive_time = std::chrono::steady_clock::now();
 
                // axis1.move(servo_power.axis1);
                // axis2.move(servo_power.axis2);
                // axis3.move(servo_power.axis3);

                const auto servo_power = receive_data->servo;

                // カメラ首振り 受信値(← -1.0 ~ 1.0 →)をサーボの角度に変換
                const float clamped_camera_lr = clamp(servo_power.camera_left_right, -1.0f, 1.0f);
                camera_left_right.move((clamped_camera_lr + 1) * deg_to_rad(135));  // 270度サーボなので135度でセンターになる

                // カメラ上下 受信値(↓ 0 ~ 1 ↑)をサーボの角度に変換
                const float clamped_camera_ud = clamp(servo_power.camera_up_down, 0.0f, 1.0f);
                camera_up_down.move((1 - clamped_camera_ud) * 1.35);  // 1.35は実験的に決めた
                return "[ OK ]";
            }
            else
            {
                return "[ NG ] Invalid json";
            }
        },
        .on_tick = [&]() {
            
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_receive_time).count();
 
            // 一定時間 受信が途絶えたら停止する。
            if (connected && elapsed < deadman_timeout_ms)
            {
                mecanum.move(target_x, target_y, target_turn);
            }
            else
            {
                mecanum.stop();
            }
        },
        .tick_interval_ms = control_interval_ms,
    });

}
