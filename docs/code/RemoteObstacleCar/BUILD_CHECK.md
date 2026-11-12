# 検証記録（今回の仕様版）

- Arduino CLI 1.6.0
- Arduino Uno / arduino:avr:uno / AVR Boards 1.8.8
- IRremote 4.4.1、Servo 1.2.2
- UNO向けコンパイル: 成功
- フラッシュ11706 / 32256 bytes
- 静的RAM623 / 2048 bytes（実行時スタックの保証ではない）
- warnings allでArduinoコアnew.cppの未使用引数警告あり。スケッチ由来の警告なし。

本番の単一.inoを模擬Arduino I/Oへ接続し、g++ C++11と-Wall -Wextra -Werrorでテスト。
15cm/5cm境界、強制後退、後方自動前進、15/10/5cm時600/350/100msのブザー周期、STOP、前後同時、方向切替100ms停止、超音波無応答、指令保持、リピート、オーバーフロー、回避3秒上限、時計の折り返しを確認。

実機の配線、各モジュールの極性、リモコン受信、実走行、停止距離は未検証。
