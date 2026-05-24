#!/bin/bash
#
# install.sh - actuator_ctrl_server を systemd サービスとして登録する
#
# 使い方:
#   ./install.sh            # ビルド + サービス登録 + 起動
#   ./install.sh --no-build # ビルドを省略して登録のみ
#
set -euo pipefail

# ---- 設定 -------------------------------------------------------------
SERVICE_NAME="actuator"
PORT="9000"

# このスクリプトが置かれているディレクトリ (= actuator_ctrl_server) を基準にする
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
BINARY="${BUILD_DIR}/pibot"

# サービスを動かすユーザー (sudo 経由なら呼び出し元、それ以外は現在のユーザー)
RUN_USER="${SUDO_USER:-$(id -un)}"

UNIT_PATH="/etc/systemd/system/${SERVICE_NAME}.service"
# ----------------------------------------------------------------------

log() { printf '\033[1;32m[install]\033[0m %s\n' "$*"; }
err() { printf '\033[1;31m[error]\033[0m %s\n' "$*" >&2; }

# ---- 1. ビルド --------------------------------------------------------
if [[ "${1:-}" != "--no-build" ]]; then
    log "ビルドします (cmake)"
    cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}"
    cmake --build "${BUILD_DIR}" -j2   # Zero 2 W のメモリを考慮して -j2
else
    log "--no-build 指定のためビルドをスキップ"
fi

if [[ ! -x "${BINARY}" ]]; then
    err "実行ファイルが見つかりません: ${BINARY}"
    err "先にビルドを通してください ( ./install.sh )"
    exit 1
fi

# ---- 2. service ファイルを生成 ---------------------------------------
# パスとユーザーを実環境に合わせて埋め込む (決め打ちにしない)
log "service ファイルを生成: ${UNIT_PATH}"
sudo tee "${UNIT_PATH}" > /dev/null <<EOF
[Unit]
Description=actuator ctrl web server
After=network.target

[Service]
Type=simple
User=${RUN_USER}
ExecStart=${BINARY} ${PORT}
Restart=on-failure
RestartSec=2

[Install]
WantedBy=multi-user.target
EOF

# ---- 3. 登録 & 起動 ---------------------------------------------------
log "systemd に登録して起動します"
sudo systemctl daemon-reload
sudo systemctl enable "${SERVICE_NAME}.service"
sudo systemctl restart "${SERVICE_NAME}.service"

# ---- 4. 結果表示 ------------------------------------------------------
sleep 1
if systemctl is-active --quiet "${SERVICE_NAME}.service"; then
    log "起動成功 ✓"
else
    err "起動に失敗しました。ログを確認してください:"
    err "  journalctl -u ${SERVICE_NAME} -n 30 --no-pager"
    exit 1
fi

cat <<EOF

────────────────────────────────────────
 ${SERVICE_NAME}.service を登録しました
────────────────────────────────────────
  実行ユーザー : ${RUN_USER}
  実行ファイル : ${BINARY}
  ポート       : ${PORT}

 よく使うコマンド:
  状態確認 : sudo systemctl status ${SERVICE_NAME}
  ログ追跡 : journalctl -u ${SERVICE_NAME} -f
  再起動   : sudo systemctl restart ${SERVICE_NAME}
  停止     : sudo systemctl stop ${SERVICE_NAME}
  自動起動を解除 : sudo systemctl disable ${SERVICE_NAME}
────────────────────────────────────────
EOF