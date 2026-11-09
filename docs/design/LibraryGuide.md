# コードライブラリの使い方

## 収録内容
統合プロジェクト5ファイル、関数抜粋と関数設計書、単独診断スケッチ9本を収録。関数抜粋は単独スケッチではなく参照用。Arduino IDEではRobotCar/RobotCar.inoを開き、同じフォルダに4つの.hを揃える。

## 実機へ合わせる手順
1. モータードライバー、端子、回路、センサー型番を確認する。本アダプターは左右各IN1/IN2/PWM型を仮定。実ドライバーが異なる場合はHardwareAdapterを変更する。
2. 診断04でIRのprotocol/address/commandを採取し、NEC/NEC2かを確認する。他形式にはRemoteAdapterの変更が必要。
3. BoardConfigのピン、極性、キー値を入力する。D0/D1、重複ピン、IRremoteとPWMのタイマー競合を避ける。
4. BUG-001の5cm未満/以下を合意してParameters.emergencyInclusiveを設定する。既定値は仮に<=。確認フラグは実際の確認後に設定する。
5. アクティブブザーか確認する。受動ブザーとtone()を使用する場合、UNOでのIRremoteとのタイマー競合を再設計する。
6. 車輪を浮かせて方向・PWM0の停止を確認し、低速・管理された範囲でテストする。

## 動作と制約
STOPと通信断は自動回避より優先。危険距離・センサー異常・前後同時検知・回避タイムアウトは保持停止。安全が連続確認でき、新しいSTOP操作を受信したら解除待ちへ移り、その後の新規操作で復帰する。回避完了後も押しっぱなしでは再始動しない。

速度120/70、通信期限350ms、測定60ms、鮮度200ms、方向切替停止100ms、回避上限1500ms、安全確認400msは全て仮の初期値。実機で応答と制動距離を評価して決定する。

pulseInは最大約15ms待機する。全処理が非ブロッキングではない。停止指令と物理制動は異なる。側方の安全や後方デジタルセンサーの断線を保証できない。IRremoteのフレーム受信・測定周期・シリアル出力による遅延は実機で測定する。

## 確認済み範囲
ホストg++ C++11の論理テストを実行。境界、時計の折り返し、欠測、古いデータ、通信断、回避、STOP優先、タイムアウト保持停止、復帰、新規操作待ちを確認。Arduino UNO向けクロスコンパイル、実センサー、配線、実走行、物理的な停止距離は未検証。既存TCの実機合否は変更しない。

ZIP展開先では `g++ -std=c++11 -IRobotCar tests/robot_logic_test.cpp -o logic-test` で論理テストを実行できる（g++が必要）。I/Oアダプターの構文検査にはローカルのArduino/IRremote模擬ヘッダーを使用した。これはUNO向けビルドや実ライブラリの検証の代わりにはならない。

## 参照資料
- Arduino pulseIn: https://docs.arduino.cc/language-reference/en/functions/advanced-io/pulseIn/
- Arduino millis: https://docs.arduino.cc/language-reference/en/functions/time/millis/
- Arduino-IRremote公式: https://github.com/Arduino-IRremote/Arduino-IRremote

APIの参照日：2026-11-10。センサーとモータードライバーの仕様は実機資料で確認する。
