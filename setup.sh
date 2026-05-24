#!/bin/bash
#
# setup.sh - 依存ツールのインストールとビルド (リポジトリ直下で実行)
#
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- 依存ツールのインストール ---
sudo apt update
sudo apt install -y git build-essential cmake

# --- pigpio (GPIO制御ライブラリ) ---
# Debian 13 (trixie) では apt に無いためソースからビルドする。
# 既にインストール済みならスキップ (べき等性のため)。
if [ -f /usr/local/lib/libpigpio.so ] || [ -f /usr/lib/libpigpio.so ]; then
    echo "pigpio はインストール済みのためスキップします。"
else
    echo "pigpio をソースからビルドします。"
    PIGPIO_DIR="$(mktemp -d)"
    git clone --depth 1 https://github.com/joan2937/pigpio.git "${PIGPIO_DIR}/pigpio"
    make -C "${PIGPIO_DIR}/pigpio"
    # make install は pigpiod (デーモン) の systemd 登録で警告を出すことがあるが、
    # 本プロジェクトは libpigpio を直接リンクして使うためデーモンは不要。
    # ライブラリとヘッダさえ入ればよいので、登録失敗は無視する。
    sudo make -C "${PIGPIO_DIR}/pigpio" install || true
    sudo ldconfig
    rm -rf "${PIGPIO_DIR}"

    # ライブラリが実際に入ったか確認
    if [ ! -f /usr/local/lib/libpigpio.so ] && [ ! -f /usr/lib/libpigpio.so ]; then
        echo "pigpio のインストールに失敗しました。"
        exit 1
    fi
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