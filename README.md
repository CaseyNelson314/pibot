# pibot

Raspberry Pi Zero 2 W で動作するロボットアーム付きメカナムロボット

## ロボットとの通信方法

### 移動量、アームの角度の送信

移動量、アームの角度等の指令値は、WebSocket を用いてロボットへ送信します。指令値を送信する PC は Raspberry Pi と同一の LAN に接続されている必要があります。WebSocket サーバーの URL、ポートは次の通りです。

```txt
ws://raspberrypi.local:9000
```

指令値は JSON 形式の文字列で送信し、形式は次の通りです。

```json
{
    "wheel": {
        "x": 0.0,    // X移動量 (-1~1)
        "y": 0.3,    // Y移動量 (-1~1)
        "turn": 0.1  // 旋回量  (-1~1)
    },
    "arm": {
        "axis1": 3.14,    // 第1関節 (rad)
        "axis2": 0.23,    // 第2関節 (rad)
        "axis3": -0.3,    // 第3関節 (rad)
        "axis4": 0.51,    // 第4関節 (rad)
        "axis5": 0.51     // 第5関節 (rad)
    }
}
```

簡易ウェブクライアントを用いて動作検証できます。セキュリティーの関係でウェブアプリは公開されておらず、自前でウェブサーバーを立てる必要があります。以下コマンドは全て Windows 上で実行します。

> ウェブサーバーの起動には bun が必要です。bun インストール方法 (Windows用)
> 
> ```sh
> powershell -c "irm bun.sh/install.ps1 | iex"
> ```

ウェブサーバー起動

```sh
cd ~/pibot/actuator_ctrl_client
bun i
bun run dev
```

起動すると次のように出力されるため、URL にブラウザでアクセスします。

```txt
  VITE xxxx  ready in 301 ms

  ➜  Local:   http://localhost:XXXXX/
  ➜  Network: use --host to expose
  ➜  press h + enter to show help
```

次のような操作画面が表示されます。

![Alt text](https://github.com/user-attachments/assets/1bff3622-15c1-4341-9044-2fdeaa45bc32)

### カメラ映像の受信

カメラの映像は WebRTC を用いて受信します。WebRTC のシグナリングサーバーの URL は次の通りです。

```txt
ws://raspberrypi.local:8000/ws
```

momo というソフトウエアを用いて配信しており、正常に配信出来ているか確認できるウェブページも同時に配信されています。momo の公式ページは[こちら](https://momo.shiguredo.jp/)です。

次のURLから閲覧できます。Raspberry Pi と同一 LAN に存在する PC のブラウザで開いてください。

<http://pibot.local:8080/html/p2p.html>

次のような画面が表示されます。

![Alt text](https://github.com/user-attachments/assets/f15ac71a-6ea5-4c73-83a7-ad9c1be68c64)

## 一から構築する手順

Raspberry Pi Zero 2 W に SSH で接続します。Raspberry Pi のターミナルが使えればよく、HDMI 接続でも実行可能です。

以下コマンドは全て Raspberry Pi 上で実行するコマンドです。また本プロジェクトはホームディレクトリに配置される前提で制作しています。

### 1. ツールチェーンインストール

```sh
sudo apt update
sudo apt upgrade
sudo apt install cmake pigpio
```

### 2. アクチュエーター制御用サーバー構築

本プロジェクトをクローン

```sh
git clone https://github.com/CaseyNelson314/pibot.git
cd ~/pibot/actuator_ctrl_server/
```

ビルド

```sh
cmake -S . -B build
cmake --build build
```

起動時に実行されるように systemd に登録

```sh
sudo cp ~/pibot/actuator_ctrl_server/actuator.service /etc/systemd/system/
sudo systemctl start  actuator.service
sudo systemctl enable actuator.service
```

> 単体で実行する場合 (9000番ポートでサーバーを起動)
> 
> ```sh
> sudo ~/pibot/actuator_ctrl_server/build/pibot 9000
> ```

### 3. カメラ映像配信サーバー構築

インストール

```sh
cd ~/pibot/camera_streaming_server/
./install.sh
```

起動時に実行されるように systemd に登録

```sh
sudo cp ~/pibot/camera_streaming_server/camera_streaming_server.service /etc/systemd/system/
sudo systemctl start  camera_streaming_server.service
sudo systemctl enable camera_streaming_server.service
```

> 本例ではインストールスクリプトでバイナリをダウンロードしますが、最新バイナリは[リリースページ](https://github.com/shiguredo/momo/releases)から取得できます。
