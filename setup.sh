#!/bin/bash

set -e
 
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
 
# 依存ツールのインストール
sudo apt update
sudo apt install -y cmake pigpio
 
# ビルド (Zero 2 W のメモリを考慮して -j2)
cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/build"
cmake --build "${SCRIPT_DIR}/build" -j2
 
echo "ビルド完了。次に ./install.sh で systemd に登録できます。"
 