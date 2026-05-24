#!/bin/bash
# actuator と camera を systemd に登録して起動する (リポジトリ直下で実行)

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RUN_USER="${SUDO_USER:-$(id -un)}"

# --- actuator サーバー ---
ACTUATOR_BIN="${ROOT_DIR}/actuator_ctrl_server/build/pibot"
if [ ! -x "${ACTUATOR_BIN}" ]; then
    echo "actuator の実行ファイルがありません: ${ACTUATOR_BIN}"
    echo "先に ./setup.sh を実行してください。"
    exit 1
fi

sudo tee /etc/systemd/system/actuator.service > /dev/null <<EOF
[Unit]
Description=actuator ctrl web server
After=network.target

[Service]
Type=simple
User=${RUN_USER}
ExecStart=${ACTUATOR_BIN} 9000
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target
EOF

# --- camera サーバー (momo) ---
MOMO_BIN="$(find "${ROOT_DIR}/camera_streaming_server" -name momo -type f | head -n 1)"
if [ -z "${MOMO_BIN}" ]; then
    echo "momo バイナリが見つかりません (camera_streaming_server 以下)"
    exit 1
fi
MOMO_DIR="$(dirname "${MOMO_BIN}")"

sudo tee /etc/systemd/system/camera_streaming_server.service > /dev/null <<EOF
[Unit]
Description=Momo Camera Streaming Server
After=network.target

[Service]
Type=simple
ExecStart=${MOMO_BIN} --force-i420 --hw-mjpeg-decoder=true --no-audio-device --use-libcamera p2p
WorkingDirectory=${MOMO_DIR}/
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target
EOF

# --- 登録して起動 ---
sudo systemctl daemon-reload
sudo systemctl enable actuator.service camera_streaming_server.service
sudo systemctl restart actuator.service camera_streaming_server.service

echo "登録完了。"
echo "  状態確認 : sudo systemctl status actuator camera_streaming_server"
echo "  ログ     : journalctl -u actuator -f"
echo "             journalctl -u camera_streaming_server -f"