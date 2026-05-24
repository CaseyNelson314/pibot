#!/bin/bash
#
# setup.sh - 依存ツールのインストールとビルド (リポジトリ直下で実行)
#
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- 依存ツールのインストール ---
sudo apt update
sudo apt install -y git build-essential cmake

# --- lgpio (GPIO制御ライブラリ) ---
# Debian 13 (trixie) では apt に無いためソースからビルドする。
# 既にインストール済みならスキップ (べき等性のため)。
if [ -f /usr/local/lib/liblgpio.so ]; then
    echo "lgpio はインストール済みのためスキップします。"
else
    echo "lgpio をソースからビルドします。"
    LG_DIR="$(mktemp -d)"
    git clone --depth 1 https://github.com/joan2937/lg.git "${LG_DIR}/lg"
    make -C "${LG_DIR}/lg"
    sudo make -C "${LG_DIR}/lg" install
    sudo ldconfig
    rm -rf "${LG_DIR}"
fi

# --- actuator サーバーのビルド ---
# Zero 2 W のメモリを考慮して -j2
cmake -S "${ROOT_DIR}/actuator_ctrl_server" -B "${ROOT_DIR}/actuator_ctrl_server/build"
cmake --build "${ROOT_DIR}/actuator_ctrl_server/build" -j2

# --- camera: momo バイナリに実行権限を付与 ---
# リポジトリ同梱のバイナリをそのまま使う
MOMO_BIN="$(find "${ROOT_DIR}/camera_streaming_server" -name momo -type f | head -n 1)"
if [ -z "${MOMO_BIN}" ]; then
    echo "momo バイナリが見つかりません (camera_streaming_server 以下)"
    exit 1
fi
chmod +x "${MOMO_BIN}"

echo "セットアップ完了。次に ./install.sh で systemd に登録できます。"