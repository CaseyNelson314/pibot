#pragma once

#include <functional>
#include <string>
#include <string_view>

struct websocket_server_config
{
    int server_port;
    std::function<void(bool)> on_server_start;
    std::function<void(void)> on_open;
    std::function<void(void)> on_close;
    std::function<std::string(std::string_view)> on_message;

    // 固定周期で呼ばれるコールバック(モーター出力・フィルタ・デッドマン等の制御用)。
    // WebSocket の受信周期に依存せず、tick_interval_ms ごとに実行される。
    std::function<void(void)> on_tick;
    int tick_interval_ms;
};

void websocket_server_start(const websocket_server_config& e);