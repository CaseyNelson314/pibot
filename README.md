# pibot

Raspberry Pi Zero 2 W で動作する、カメラ付きメカナムホイールロボット

Web ブラウザから遠隔操縦でき、カメラ映像を WebRTC で受信できる。

## 構成概要

- **アクチュエーター制御サーバー** (`actuator_ctrl_server/`)
  
  WebSocket で操縦指令を受け、メカナムホイール 4 輪とサーボ 5 軸を制御

- **カメラ映像配信サーバー** (`camera_streaming_server/`)
  
  [momo](https://momo.shiguredo.jp/) を用いて WebRTC でカメラ映像を配信する。

- **操縦用ウェブクライアント** (`actuator_ctrl_client/`)
  
  動作検証用。ジョイスティックで並進、スライダーで旋回・アームを操作する。

> [!NOTE]
> 本プロジェクトは Raspberry Pi のホームディレクトリ (`~/pibot`) に配置される前提です。
> ホスト名は `pibot` を想定しており、各 URL は `pibot.local` で記載しています。
> 環境に合わせて読み替えてください。

---

## セットアップ

Raspberry Pi に SSH で接続して作業します（HDMI 直結でも可）。セットアップは 2 つのスクリプトに分かれています。特に指定のない限り、記載のコマンドは Raspberry Pi 上で実行します。

```sh
git clone https://github.com/CaseyNelson314/pibot.git
cd ~/pibot
chmod +x setup.sh install.sh

./setup.sh     # 依存ツールのインストールとビルド
./install.sh   # systemd への登録と起動
```

### setup.sh が行うこと

- 依存ツール (`git` `build-essential` `cmake`) のインストール
- pigpio のソースビルド
- アクチュエーター制御サーバーのビルド
- カメラ配信用 momo バイナリへの実行権限付与

### install.sh が行うこと

- `actuator.service` と `camera_streaming_server.service` を生成
- systemd へ登録し、起動・自動起動を有効化

各サービスは Raspberry Pi の起動時に自動的に立ち上がります。

## ロボットとの通信方法

### 移動量・アームの角度の送信

指令値は WebSocket で送信します。送信元の PC は Raspberry Pi と同一 LAN に接続されている必要があります。

送信先：

```txt
ws://pibot.local:9000
```

指令値は JSON 形式の文字列で送信します。範囲外の値はクランプされます。例：

```json
{
  "wheel": {
    "x": 0,    // -1~1 の範囲で、前が正、後ろが負
    "y": 0,    // -1~1 の範囲で、右が正、左が負
    "turn": 0  // -1~1 の範囲で、右回転が正、左回転が負
  },
  "servo": {
    "camera_left_right": 0.07,   // -1~1 の範囲で、左が負、右が正
    "camera_up_down": 0.56       //  0~1 の範囲で、下が負、上が正
  }
}
```


### 制御方式について

サーバーは受信した指令値を保持し、一定周期（100 Hz）の制御ループでモーターを駆動します。送信側の通信周期に依存しません。

各モーターは出力の平滑化を行い、急な指令値変化による突入電流を抑制します。移動平均を用いて平滑化しており、窓幅は 20 で、制御周期 100 Hz では約 0.2 秒かけて目標値まで立ち上がります。

最後の指令受信から一定時間（500 ms）通信が途絶えると、自動的に停止します。

### 操縦用ウェブクライアント

簡易ウェブクライアントで動作確認できます。ウェブアプリは公開していないため、自前でウェブサーバーを起動します。以下は Windows 上での実行例です。

> ウェブサーバーの起動には bun が必要です。Windows で bun をインストールしていない場合は、PowerShell で次を実行してインストールしてください。
>
> ```sh
> powershell -c "irm bun.sh/install.ps1 | iex"
> ```

```sh
cd ~/pibot/actuator_ctrl_client
bun i
bun run dev
```

表示された URL（`http://localhost:XXXXX/`）にブラウザでアクセスします。並進はジョイスティック、旋回はスライダーで操作します（いずれも手を離すと中立に戻ります）。

### カメラ映像の受信

カメラ映像は momo を用いて WebRTC で配信されます。同一 LAN の PC のブラウザから、動作確認用ページにアクセスできます。

```txt
http://pibot.local:8080/html/p2p.html
```

WebRTC のシグナリングサーバーの URL は次の通りです。通信方法は momo のドキュメントを参照してください。

```txt
ws://pibot.local:8000/ws
```

## 開発時のヒント・トラブルシューティング

### ソースを編集してビルドし直す

```sh
cd ~/pibot/actuator_ctrl_server
cmake --build build -j2
sudo systemctl restart actuator
```

### サービスの状態を確認する

```sh
sudo systemctl status actuator camera_streaming_server
journalctl -u actuator -f                 # ログをリアルタイム表示
journalctl -u camera_streaming_server -f
```

### `pibot` 起動時に「don't have permission to run」と出る／サービスが起動しない

pigpio は GPIO アクセスに root 権限を必要とします。`actuator.service` は root で起動する設定です。手動実行する場合も `sudo` を付けてください。

```sh
sudo ~/pibot/actuator_ctrl_server/build/pibot 9000
```

### カメラサービスが起動しない

カメラが認識していない場合に起こります。配線を確認してください。認識されているかは次で確認できます。

```sh
rpicam-hello --list-cameras
```

### モーターを動かすとバッテリーがエラー表示（点滅）になる／Raspberry Pi が落ちる

電源容量不足、または電源の共有が原因です。

### 電源を直接切った後起動しなくなった

まれに SD カードのファイルシステムが破損することがあります。

1. SD カードを PC に挿し、`bootfs` パーティションが見えるか確認します。
   - 見える場合 SD は生きています。Raspberry Pi Imager で OS を焼き直せば復旧します。
   - 見えない場合 SD カードが物理的に破損しています。新しい SD に焼き直してください。
2. 焼き直し後は `./setup.sh && ./install.sh` で復旧します。

> [!NOTE]
> OS を焼き直すと SSH のホスト鍵が変わり、再接続時に警告が出ます。PC 側で次を実行してから再接続してください。
>
> ```sh
> ssh-keygen -R pibot.local
> ```

### VS Code の Remote-SSH が接続できない

Zero 2 W はメモリが少なく、VS Code Server の常駐が不安定です。`rsync` での差分転送を推奨します。
