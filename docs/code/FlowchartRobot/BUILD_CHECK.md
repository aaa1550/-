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

## 追加資料反映版の検証

- BoardPins.hの既定配線: UNO向けコンパイル成功。
- USE_PHOTO_ECHO_WIRING=true（Echo=D2、右後方=A5）: UNO向けコンパイル成功。フラッシュ10682 bytes、静的RAM621 bytes。
- わざとEchoと右後方を両方D2にした構成: `Pin collision`でコンパイルが失敗することを確認。
- TrackerMonitor（画像5.jpgのIR1〜IR5=A0〜A4）: UNO向けコンパイル成功。フラッシュ2316 bytes、静的RAM200 bytes。
- モーターPWMはD5/D6に制限し、ServoのTimer1とIRremoteのTimer2との競合を設定段階で回避。

画像の配線情報の照合とクロスコンパイルであり、現物センサーの極性・取り付け・検知距離の確認結果ではありません。
