#include <App.h>
#include <cstring>
#include "websocket.hpp"

void websocket_server_start(const websocket_server_config& conf)
{
    struct UserData
    {};

    uWS::App().ws<UserData>("/", {
        .open = [&conf](auto */*ws*/) {
            conf.on_open();
        },
        .message = [&conf](auto *ws, std::string_view msg, uWS::OpCode opCode) {
            const auto response = conf.on_message(msg);
            if (response.size())
                ws->send(response, opCode, false);
        },
        .close = [&conf](auto */*ws*/, int /*code*/, std::string_view /*message*/) {
            conf.on_close();
        }
    }).listen(conf.server_port, [&conf](auto *token) {
        conf.on_server_start(static_cast<bool>(token));

        // サーバー起動成功時(イベントループが確立済み)に固定周期タイマーを登録する。
        // listen コールバックは run() 実行中に呼ばれるため、ここで Loop::get() すれば
        // 正しく現在のループを取得できる。uWS と同一ループ上で発火するので、
        // on_message と同時実行されず、共有データの排他制御は不要。
        if (token && conf.on_tick && conf.tick_interval_ms > 0)
        {
            auto *loop = reinterpret_cast<struct us_loop_t *>(uWS::Loop::get());
            struct us_timer_t *timer = us_create_timer(loop, 0, sizeof(void *));

            const auto *tick_ptr = &conf.on_tick;
            std::memcpy(us_timer_ext(timer), &tick_ptr, sizeof(void *));

            us_timer_set(timer,
                [](struct us_timer_t *t) {
                    const std::function<void(void)> *cb;
                    std::memcpy(&cb, us_timer_ext(t), sizeof(void *));
                    (*cb)();
                },
                conf.tick_interval_ms,    // 初回発火までの ms
                conf.tick_interval_ms);   // 繰り返し間隔 ms
        }
    }).run();
}