# zmk-driver-iqs9151-rpc

ShiniNetさんが作成された [zmk-driver-iqs9151](https://github.com/ShiniNet/zmk-driver-iqs9151) をベースに、DYA Studio / ZMK Studio Custom RPC とWebUIを追加した派生版です。
IQS9151トラックパッドモジュールをZMKで使用し、カーソル移動/タップ/スクロール/ピンチインアウトや複数指ジェスチャなどの操作を扱えるようになります。
この派生版では、Studio対応ファームウェアからIQS9151パラメータを読み込み・変更・リセットできるWebUIを同梱しています。

トラックパッドモジュール本体はShiniNetさんの[Booth（準備中）](https://shininet.booth.pm/)より入手可能です。

<img width="600"  alt="image" src="https://github.com/user-attachments/assets/76c1e221-bab2-4d7d-9250-408a9b767e39" />

## ドライバの特徴

- IQS9151トラックパッドの入力をZMKへ統合
- 1本指カーソル移動、タップ&タップドラッグ
- 2本指スクロール（縦/横）、タップ&タップドラッグ、ピンチインアウト
- 3本指タップ&タップドラッグ、スワイプ系ジェスチャ（設定に応じて有効化）
- 滑らかな慣性カーソル/スクロール対応
- ZMKのキーマップ連携（レイヤーごとに動作の割り当て可能）
- カーソルやスクロールの速度をリアルタイムに調整可能（電源OFFで設定が消えない）
- DYA Studio / ZMK Studio Custom RPC経由でIQS9151パラメータをWebUIから調整可能

## この派生版で追加したもの

- `dya__iqs9151` / `zmk__iqs9151` Custom Studio RPCサブシステム
- IQS9151パラメータ調整用WebUI
- WebUIデモモード
- `get_config` / `set_config` / `reset_config` RPC
- 一部パラメータのランタイム反映


## クイックスタート

ここでは最小構成での導入手順を示します。
前提となるZMKの基本構成・ビルド手順は本ドキュメントでは取り扱いません。

### 1. `west.yml` にモジュールを追加

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: te9no                                   #<---add this
      url-base: https://github.com/te9no            #<---add this

  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.3.0
      import: app/west.yml

    - name: zmk-driver-iqs9151-rpc                  #<---add this
      remote: te9no                                 #<---add this
      revision: main                                #<---add this
      path: modules/zmk-driver-iqs9151-rpc          #<---add this

  self:
    path: config
```

### 2. `.conf` に必要設定を追加

```conf
CONFIG_I2C=y
CONFIG_ZMK_POINTING=y
CONFIG_ZMK_POINTING_SMOOTH_SCROLLING=y
CONFIG_INPUT_IQS9151=y
CONFIG_ZMK_STUDIO=y
CONFIG_INPUT_IQS9151_STUDIO_RPC=y
```

必要に応じてトラックパッドの調整やジェスチャーON/OFFや閾値の設定を追加してください（ShiniNetさんの[IQS9151 Driver Kconfig Reference](https://github.com/ShiniNet/zmk-driver-iqs9151/blob/main/documents/iqs9151_kconfig_reference.md)参照）。

### 3. DTS(overlay) にIQS9151ノードを追加（Xiao BLE且つセントラル側の例）

```dts
&xiao_i2c {
    status = "okay";

    iqs9151: iqs9151@56 {
        compatible = "azoteq,iqs9151";
        reg = <0x56>;
        status = "okay";
        irq-gpios = <&gpio1 11 (GPIO_ACTIVE_LOW)>;
    };
};

/ {
    trackpad_listener {
        compatible = "zmk,input-listener";
        device = <&iqs9151>;
        status = "okay";
    };
};
```
必要に応じてトラックパッドの速度調整や仮想キー連携を行う場合は、Behavior / Input Processor を追加してください（ShiniNetさんの[Behavior / Input Processor Reference](https://github.com/ShiniNet/zmk-driver-iqs9151/blob/main/documents/behavior_input_processor_reference.md)参照）。

### 4. 物理配線

- `SDA` -> MCUのI2C SDA（Xiao BLEの場合D4）
- `SCL` -> MCUのI2C SCL（Xiao BLEの場合D5）
- `DR` -> DTSの`irq-gpios`で指定したGPIO入力（今回の例ではGPIO1.11=D6）
- `RST` -> MCUのRESETピン（省略可能）
- `3.3V` -> 3.3V電源
- `GND` -> GND

※SDA/SCL/DRのプルアップ抵抗はトラックパッド側に4.7KΩ実装済みなので不要。

<img width="1816" height="1334" alt="2026-03-27_16h37_57" src="https://github.com/user-attachments/assets/95b96d59-c3e7-432c-b41d-6f99093fd536" />

> [!WARNING]
> - 以下の画像の例はDTSの`irq-gpios`へGPIO0.02=D0に設定したときの配線例です。
> - （DRに接続した白ワイヤーをXiaoBLEのD0に接続しています。）

<img width="1083" height="600" alt="2026-03-27_15h43_31" src="https://github.com/user-attachments/assets/ba32499c-6387-4ba4-9f93-6c0c47308d7e" />

### 5. トラックパッドの接続と組立て

- FFCにトラックパッドを接続します。
- トラックパッドに厚さ1mm程度の任意のオーバーレイを貼り付けます。（アクリル板、プラ版、PLA、PETGなど。）

> [!WARNING]
> - FFCは青い面が手前側に見える向きに差し込んでください。
> - 付属のブレークアウトボードを使用する場合、FFCは6PINの接点が反転しているタイプを使用してください。
> - トラックパッドにオーバーレイを取り付けない裸の状態で使用すると、センサー飽和を起こして正しく動作しません。

<img width="1144" height="586" alt="2026-03-27_16h09_46" src="https://github.com/user-attachments/assets/63f06b36-9f48-481c-8e7e-c03caf2363a6" />

### 6. 動作確認（デフォルト機能）

- ビルド/書き込み後、1～2本指スワイプによるポインタ移動とスクロールを確認
- 1～3本指タップによる左右中クリック、タップドラッグを確認
- 3本指左右スワイプによるマウスボタン4-5(進む戻る)の出力を確認

※更なる動作をキーマップから設定できるようにするにはコンフィグ及びDTSの設定が必要です。

## DYA Studio / WebUI

このRPC版では、DYA Studio向けに `dya__iqs9151` Custom Studio RPCサブシステムを追加しています。
互換用に `zmk__iqs9151` もadvertiseします。
Studio対応ファームウェアを接続すると、WebUIからIQS9151の一部パラメータを読み込み・変更・初期値へリセットできます。
IQS9151がsplit peripheral側にある場合は、ZMK split relay eventでcentralからperipheralへRPC要求を中継します。

### 調整できる項目

- X/Y resolution
- ATI target count
- XY dynamic filter bottom/top speed
- XY dynamic filter bottom beta
- 1F / 2F / 3F tap enable
- horizontal / vertical scroll enable
- pinch enable
- cursor / scroll inertia enable

### ファームウェア側の設定

```conf
CONFIG_ZMK_STUDIO=y
CONFIG_INPUT_IQS9151_STUDIO_RPC=y
CONFIG_ZMK_SPLIT_RELAY_EVENT=y
```

production firmwareでは `https://te9no.github.io/zmk-driver-iqs9151-rpc/` をadvertiseします。
WebUI開発時は `http://localhost:5173` に一時変更してください。

### WebUIの起動

```sh
cd .west-workspace/modules/zmk-driver-iqs9151-rpc/web
npm install
npm run dev
```

production build:

```sh
npm run build
```

`npm run build` は生成済みの `web/src/proto` を使います。
`proto/zmk/iqs9151/iqs9151.proto` を変更した場合だけ、次を実行してください。

```sh
npm run generate
```

### デモモード

WebUI上部の `Enable Demo` を押すと、実機に接続しなくても画面表示とパラメータ編集の動作を確認できます。
デモモードではRPC通信は行わず、ブラウザ内のローカル状態だけを更新します。


## 応用編（ドキュメント整備中...）

- ZMKキーマップと連携しKeymap EditorやZMK Studioからトラックパッドの動作を変更する
- キー入力によってトラックパッドのカーソル(スクロール)速度を動的に変更する
- スプリットキーボード構成への組み込み。ダブルトラックパッド化

> [!TIP]
> - 具体的な実装例は[ LaLapad Gen2のファームウェアリポジトリ](https://github.com/ShiniNet/zmk-config-LalaPadGen2)を参照してください。
> - トラックパッドの機能や使い方については[LaLapag Gen2の使い方](https://github.com/ShiniNet/LaLaPadGen2/blob/main/guide/HowtoUse.md#%E3%83%88%E3%83%A9%E3%83%83%E3%82%AF%E3%83%91%E3%83%83%E3%83%89%E3%81%AE%E6%A9%9F%E8%83%BD)を参考にしてください。


## 対応環境

- ZMK Firmware `v0.3.0` 以上
- 確認済みMCU/ボード: `Seeed XIAO BLE` / `Seeed XIAO BLE Plus`
- 他のnRF52840系ボードでも動作する可能性はありますが、未検証です（利用は自己責任）。
- 最低でも `SDA/SCL/DR` 用GPIO + `3.3V/GND` が利用可能であること。

## 注意事項

- `TPS65` など同社の汎用トラックパッド等では動作しません。


## 免責事項・その他

- 本リポジトリはShiniNetさんのIQS9151ドライバーをベースにした派生版です。
- 本派生版は継続開発中のため、意図しない動作の変更や不具合が発生する可能性があります。
- 意図しない更新を避けたい場合、ユーザー側でフォークしたバージョンを使用するなど対策してください。
- 本ソフトウェアは現状有姿（as is）で提供され、利用・導入・運用は利用者の自己責任です。
- RPC/WebUIまたは本派生版固有の問題は、このリポジトリへISSUEを起こしてください。
- 元ドライバー由来と思われる問題は、ShiniNetさんのオリジナルリポジトリの状況も確認してください。
