# pibot

Raspberry Pi Zero 2 W で動作する、カメラ付きメカナムホイールロボット。

WebScketで制御でき、カメラ映像を WebRTC で受信できる。

## 構成

- **アクチュエーター制御サーバー** (`actuator_ctrl_server/`) — C++ 製。WebSocket で操縦指令を受け、メカナムホイール 4 輪とサーボ 2 軸を制御する。
- **カメラ映像配信サーバー** (`camera_streaming_server/`) — [momo](https://momo.shiguredo.jp/) を用いて WebRTC でカメラ映像を配信する。
- **操縦用ウェブクライアント** (`actuator_ctrl_client/`) — 動作検証用。ジョイスティックで並進、スライダーで旋回・アームを操作する。

> [!NOTE]
> 本プロジェクトは Raspberry Pi のホームディレクトリ (`~/pibot`) に配置される前提です。
> ホスト名は `pibot` を想定しており、各 URL は `pibot.local` で記載しています。
> 環境に合わせて読み替えてください。

---

## セットアップ

Raspberry Pi に SSH で接続して作業します（HDMI 直結でも可）。
セットアップは 2 つのスクリプトに分かれています。

```sh
git clone https://github.com/CaseyNelson314/pibot.git
cd ~/pibot
chmod +x setup.sh install.sh

./setup.sh     # 依存ツールのインストールとビルド
./install.sh   # systemd への登録と起動
```

### setup.sh が行うこと

- 依存ツール (`cmake`等) のインストール
- pigpio のソースビルド
- アクチュエーター制御サーバーのビルド
- カメラ配信用 momo バイナリへの実行権限付与

### install.sh が行うこと

- `actuator.service` と `camera_streaming_server.service` を生成
- systemd へ登録し、起動・自動起動を有効化

登録後、各サービスは Raspberry Pi の起動時に自動的に立ち上がります。

## ロボットとの通信方法

### 移動量・アームの角度の送信

指令値は WebSocket で送信します。送信元の PC は Raspberry Pi と同一 LAN に接続されている必要があります。

```txt
ws://pibot.local:9000
```

指令値は JSON 形式の文字列で送信します。

```json
{
    "wheel": {
        "x": 0.0,    // X 移動量 (-1〜1)
        "y": 0.3,    // Y 移動量 (-1〜1)
        "turn": 0.1  // 旋回量   (-1〜1)
    },
    "servo": {
        "camera_left_right": 0.1,    // カメラの首振り -1~1
        "camera_up_down": 0.23,       // カメラの上下  0~1
    }
}
```

> [!NOTE]
> 範囲外の値は無視されます。

### 制御方式について

サーバーは受信した指令値を保持し、**一定周期（100 Hz）の制御ループ**でモーターを駆動します。送信側の通信周期に依存せず、安定した制御が行われます。

- 突入電流対策

各モーターはが出力値の平滑化をし、急なデューティ変化による突入電流を抑制します。移動平均フィルタを用いて平滑化を行っており、窓枠は20です。制御周期 100 Hz では約 0.2 秒かけて目標値まで立ち上がります。

- 通信切断検出

最後の指令受信から一定時間（500 ms）通信が途絶えると、自動的に停止します。通信切断時の暴走を防ぎます。

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

## 開発時のヒント

### ソースを編集してビルドし直す

```sh
cd ~/pibot/actuator_ctrl_server
cmake --build build -j2     # Zero 2 W のメモリを考慮して -j2
sudo systemctl restart actuator
```

> [!NOTE]
> ビルドは `-j2` を推奨します。Zero 2 W はメモリが少なく（約 400 MB）、
> 並列数を上げると nlohmann/json や uWebSockets のコンパイルでメモリ不足になります。

### PC で編集して Raspberry Pi に転送する

Zero 2 W は非力なため、VS Code の Remote-SSH は安定しません。PC で編集し、`rsync` で差分転送してから Pi 上でビルドする運用が快適です。

```sh
# PC 側で実行
rsync -avz --exclude 'build/' --exclude '.git/' \
  ./actuator_ctrl_server/ pibot@pibot.local:/home/pibot/pibot/actuator_ctrl_server/
```

## トラブルシューティング

### サービスの状態を確認する

systemd を用いて起動時に立ち上がるように設定しています。サービスの状態やログは次のコマンドで確認できます。

```sh
sudo systemctl status actuator camera_streaming_server
journalctl -u actuator -f                 # ログをリアルタイム表示
journalctl -u camera_streaming_server -f
```

### `apt install pigpio` が `has no installation candidate` で失敗する

Debian 13 (trixie) では pigpio が apt に存在しません。`setup.sh` がソースからビルドします。手動でビルドする場合は次の通りです。

```sh
git clone --depth 1 https://github.com/joan2937/pigpio.git
cd pigpio && make && sudo make install && sudo ldconfig
```

### ビルドは通るが `undefined reference to 'gpioInitialise'` 等のリンクエラーが出る

CMake のキャッシュが古い可能性があります。`build` を作り直してください。

```sh
cd ~/pibot/actuator_ctrl_server
rm -rf build
cmake -S . -B build
cmake --build build -j2
```

### `pibot` 起動時に「don't have permission to run」と出る／サービスが起動しない

pigpio は GPIO アクセスに root 権限を必要とします（`/dev/mem` を使用するため）。`actuator.service` は root で起動する設定です。手動実行する場合も `sudo` を付けてください。

```sh
sudo ~/pibot/actuator_ctrl_server/build/pibot 9000
```

### カメラサービスが `status=127` や `The following argument was not expected: test` で起動しない

momo のバージョンによって引数仕様が異なります。新しい momo では末尾の `test` サブコマンドが廃止されています。起動コマンドは次の通りです（`test` を付けない）。

```sh
cd ~/pibot/camera_streaming_server/momo-*/
./momo --force-i420 --hw-mjpeg-decoder=true --no-audio-device --use-libcamera
```

カメラが認識されているかは次で確認できます。

```sh
rpicam-hello --list-cameras
```

### モーターを動かすとバッテリーがエラー表示（チカチカ）になる／Raspberry Pi が落ちる

**電源容量不足、または電源の共有が原因です。** モーターの突入電流で 5V ラインの電圧が降下し、過電流保護が働いたり、Raspberry Pi がブラウンアウト（電圧不足で再起動）します。

対策（効果の大きい順）:

1. **モーター電源と Raspberry Pi 電源を分離する**（最も効果的）
2. モーター電源にバルクコンデンサ（470〜1000 µF 程度）を追加し、突入電流を吸収する
3. 移動平均フィルタの窓幅を広げて、出力の立ち上がりを緩やかにする（`motor.hpp` の `moving_average<20>`）
4. より大電流を供給できる電源に変更する

> [!WARNING]
> 移動平均フィルタは突入電流のピークを抑えますが、電源容量不足そのものは解決しません。
> 窓幅を広げても改善しない場合は、電源の分離・増強が必要です。

### SSH 接続中に突然切断される／モーター動作時に不安定になる

上記の電源問題と同じ原因のことがあります。モーター動作時の電圧降下で Raspberry Pi が再起動している可能性があります。電源を確認してください。

無線が金属やバッテリーに囲まれて電波が弱っている可能性もあります。Zero 2 W のアンテナ部（基板端）を金属部品から離して配置してください。

### 電源を直接切った後、起動しなくなった

シャットダウンせずに電源を切ると、SD カードのファイルシステムが破損することがあります。

1. SD カードを PC に挿し、`bootfs` パーティションが見えるか確認します。
   - 見える → SD は生きています。論理破損のため、Raspberry Pi Imager で焼き直せば復旧します。
   - 見えない／フォーマットを要求される → SD カードが物理的に破損しています。新しい SD に焼き直してください。
2. 焼き直し後は `./setup.sh && ./install.sh` で復旧します。
3. 再発防止のため、**overlayfs の有効化**（前述）を推奨します。

> [!NOTE]
> OS を焼き直すと SSH のホスト鍵が変わり、再接続時に
> `REMOTE HOST IDENTIFICATION HAS CHANGED` の警告が出ます。
> PC 側で次を実行してから再接続してください。
>
> ```sh
> ssh-keygen -R pibot.local
> ```

### VS Code の Remote-SSH が接続できない

Zero 2 W はメモリが少なく（約 400 MB）、VS Code Server の常駐が不安定です。`rsync` での差分転送（前述）を推奨します。どうしても使う場合は swap を増やすと改善することがあります。