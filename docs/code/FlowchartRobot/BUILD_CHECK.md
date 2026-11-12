# 検証記録

- 対象: FlowchartRobot.ino / FlowchartLogic.h
- Arduino CLI 1.6.0
- ボード: arduino:avr:uno
- Arduino AVR Boards 1.8.8
- Arduino-IRremote 4.4.1
- Servo 1.2.2
- コンパイル: 成功
- フラッシュ: 10682 / 32256 bytes
- 静的RAM: 621 / 2048 bytes（残り1427 bytes。実行時スタックの保証ではない）
- `--warnings all`でArduinoコアnew.cppの未使用引数警告あり。本スケッチ由来の警告はなし。

制御テスト: g++ C++11、`-Wall -Wextra -Werror`で成功。
距離境界、指定速度、前後回避、100ms停止、STOP、前後同時、測定失敗、通信断、新規操作待ち、回避上限、時計の折り返しを確認。

実機での書き込み、配線、センサー、リモコン受信、走行、制動距離は未検証。
