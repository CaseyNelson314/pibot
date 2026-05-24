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

    
    // ── 制御周期の設定 ──────────────────────────────────────
    // WebSocket の受信周期に依存せず、一定周期でモーターを更新する。
    // これにより mecanum_wheel 内の移動平均フィルタが「実時間ベース」で効き、
    // 送信頻度が変わってもフィルタの立ち上がり時間が一定になる。
    constexpr int  control_interval_ms = 10;    // 100Hz でモーター更新
    constexpr long deadman_timeout_ms  = 500;   // この時間 受信が無ければ停止
 
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
                // モーターは目標値を保存するのみ。実際の駆動は on_tick が固定周期で行う。
                target_x = receive_data->wheel.x;
                target_y = receive_data->wheel.y;
                target_turn = receive_data->wheel.turn;
                last_receive_time = std::chrono::steady_clock::now();
 
                // サーボは突入電流と無関係なため即時反映(servo 側でジッタ抑制済み)。
                const auto arm_power = receive_data->arm;
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
        },
        .on_tick = [&]() {
            // デッドマンスイッチ: 一定時間 受信が途絶えたら停止する。
            // 通信切断・フリーズ時に最後の指令で走り続ける事故を防ぐ。
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - last_receive_time).count();
 
            if (connected && elapsed < deadman_timeout_ms)
            {
                // 固定周期で move を呼ぶ。mecanum_wheel 内の移動平均が
                // この一定周期で更新されるため、突入電流が時間方向に分散する。
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
