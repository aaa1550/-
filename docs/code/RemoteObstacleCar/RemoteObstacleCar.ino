/*
 * OSOYOO Robot Car / Arduino UNO / Model X
 *
 * 【操作】方向ボタンを長押ししている間だけ走行。離すと停止。
 * 【前方】15cm以内で警告音。近づくほど速く鳴り、5cm以下で後退。
 * 【後方】障害物を検知したら前進。
 * 【停止】OK、長押し終了、センサー異常、前後同時検知を優先。
 *
 * 独自ヘッダー不要の1ファイル版。実機走行は未検証。
 */
#include <Arduino.h>

// ============================================================================
// 1. ライブラリ
// ============================================================================
#define IR_USE_AVR_TIMER2
#include <IRremote.hpp>  // Arduino-IRremote 4.4.1
#include <Servo.h>       // Servo 1.2.2

// ============================================================================
// 2. ピン設定
// ============================================================================
// 元のcode.txtから、右ENAはD9→D5、サーボ信号はD5→D9へ変更。
// ServoとIRremoteのタイマーと競合しないよう、速度PWMはD5/D6を使用。
#define IR_RECEIVE_PIN      4
#define speedPinR           5
#define RightDirectPin1    12
#define RightDirectPin2    11
#define speedPinL           6
#define LeftDirectPin1      7
#define LeftDirectPin2      8
#define BUZZ_PIN           13
#define LFSensor_bl         0  // D0を使うためSerialは使用しない
#define LFSensor_br         2
#define Echo_PIN            3
#define Trig_PIN           10
#define SERVO_PIN           9
#define REAR_ACTIVE_LEVEL LOW
#define BUZZ_ACTIVE_LEVEL LOW

// ============================================================================
// 3. リモコン・距離・速度・時間の設定
// ============================================================================
#define IR_ADVANCE         24
#define IR_BACK            82
#define IR_RIGHT           90
#define IR_LEFT             8
#define IR_STOP            28

#define WARNING_DISTANCE_CM 15.0f
#define REVERSE_DISTANCE_CM  5.0f
#define RELEASE_DISTANCE_CM 15.0f

// PWMは0〜255。実機で動き出す速度と方向を確認して調整する。
#define ADVANCE_SPEED     128
#define BACK_SPEED         77
#define TURN_SPEED         77
#define AVOID_SPEED        80

#define HOLD_TIMEOUT_MS    250UL  // 最終受信からこの時間途切れたら停止
#define SENSOR_PERIOD_MS    60UL
#define ECHO_TIMEOUT_US  15000UL  // この値だけ単位はμs
#define DIRECTION_STOP_MS  100UL
#define AUTO_MAX_MS       3000UL
#define BUZZ_SLOW_MS       600UL
#define BUZZ_FAST_MS       100UL
#define BUZZ_ON_MS          35UL

// 設定ミスをコンパイル時に検出する。
static_assert(speedPinR == 5 && speedPinL == 6,
              "UNO motor PWM must use D5/D6 with Servo and IRremote");
static_assert(Echo_PIN != LFSensor_bl && Echo_PIN != LFSensor_br,
              "Echo and rear sensor pins must be different");
static_assert(REVERSE_DISTANCE_CM > 0 &&
              WARNING_DISTANCE_CM > REVERSE_DISTANCE_CM &&
              RELEASE_DISTANCE_CM >= WARNING_DISTANCE_CM,
              "Invalid distance thresholds");
static_assert(BUZZ_ON_MS < BUZZ_FAST_MS && BUZZ_FAST_MS <= BUZZ_SLOW_MS,
              "Invalid buzzer timing");
static_assert(HOLD_TIMEOUT_MS > 0 && HOLD_TIMEOUT_MS < 0x80000000UL,
              "Invalid hold timeout");
static_assert(ADVANCE_SPEED >= 0 && ADVANCE_SPEED <= 255 &&
              BACK_SPEED >= 0 && BACK_SPEED <= 255 &&
              TURN_SPEED >= 0 && TURN_SPEED <= 255 &&
              AVOID_SPEED >= 0 && AVOID_SPEED <= 255,
              "PWM speed must be 0..255");

// ============================================================================
// 4. 車両の状態
// ============================================================================
enum DN {
  GO_ADVANCE,
  GO_BACK,
  GO_RIGHT,
  GO_LEFT,
  STOP_STOP
};

enum AvoidMode {
  AVOID_NONE,
  AVOID_FRONT,
  AVOID_REAR
};

Servo headServo;
DN Remote_Num = STOP_STOP;       // リモコンからの指令
DN Drive_Num = STOP_STOP;        // 最終的に実行する動作
AvoidMode avoidMode = AVOID_NONE;
int DriveSpeed = 0;

// センサーの状態
float distanceCm = -1.0f;
bool distanceValid = false;
bool sensorSampled = false;
int front = 0;
int back = 0;
uint32_t sensorAt = 0;
uint32_t avoidStarted = 0;

// 長押しの状態。同じリモコンの有効な繰り返し信号だけで延長する。
bool holdActive = false;
uint32_t lastRemoteAt = 0;
uint16_t lastRemoteCommand = 0;
uint16_t lastRemoteAddress = 0;
decode_type_t lastRemoteProtocol = UNKNOWN;

// ブザーとモーター切り替えの状態
bool buzzerRunning = false;
uint32_t buzzerCycleAt = 0;
int appliedRight = 0;
int appliedLeft = 0;
bool directionWaiting = false;
uint32_t directionStoppedAt = 0;

// ============================================================================
// 5. モーターの基本操作
// ============================================================================
void setMotors(int rightSpeed, int leftSpeed) {
  digitalWrite(RightDirectPin1, rightSpeed > 0 ? HIGH : LOW);
  digitalWrite(RightDirectPin2, rightSpeed < 0 ? HIGH : LOW);
  digitalWrite(LeftDirectPin1, leftSpeed > 0 ? HIGH : LOW);
  digitalWrite(LeftDirectPin2, leftSpeed < 0 ? HIGH : LOW);

  analogWrite(speedPinR, rightSpeed < 0 ? -rightSpeed : rightSpeed);
  analogWrite(speedPinL, leftSpeed < 0 ? -leftSpeed : leftSpeed);

  appliedRight = rightSpeed;
  appliedLeft = leftSpeed;
}

void go_Advance() {
  setMotors(DriveSpeed, DriveSpeed);
}

void go_Back() {
  setMotors(-DriveSpeed, -DriveSpeed);
}

void go_Right() {
  setMotors(-DriveSpeed, DriveSpeed);
}

void go_Left() {
  setMotors(DriveSpeed, -DriveSpeed);
}

void stop_Stop() {
  analogWrite(speedPinR, 0);
  analogWrite(speedPinL, 0);

  digitalWrite(RightDirectPin1, LOW);
  digitalWrite(RightDirectPin2, LOW);
  digitalWrite(LeftDirectPin1, LOW);
  digitalWrite(LeftDirectPin2, LOW);

  appliedRight = 0;
  appliedLeft = 0;
}

void resetToStop() {
  Remote_Num = STOP_STOP;
  Drive_Num = STOP_STOP;
  DriveSpeed = 0;
  avoidMode = AVOID_NONE;
  holdActive = false;
}

// ============================================================================
// 6. 長押しリモコン操作
// ============================================================================
// 赤外線リモコンは「離した」という信号を送らないため、受信間隔で判断する。
void updateRemoteHold() {
  if (!holdActive) {
    return;
  }

  const uint32_t elapsed = uint32_t(millis() - lastRemoteAt);
  if (elapsed >= HOLD_TIMEOUT_MS) {
    resetToStop();
  }
}

DN decodeRemoteDirection(uint16_t command) {
  switch (command) {
    case IR_ADVANCE: return GO_ADVANCE;
    case IR_BACK:    return GO_BACK;
    case IR_RIGHT:   return GO_RIGHT;
    case IR_LEFT:    return GO_LEFT;
    default:         return STOP_STOP;
  }
}

bool isSameRemote(uint16_t address, decode_type_t protocol) {
  // NECの長押しがNEC2として報告される場合も、同じ通信方式として扱う。
  const bool currentIsNEC = protocol == NEC || protocol == NEC2;
  const bool previousIsNEC = lastRemoteProtocol == NEC || lastRemoteProtocol == NEC2;
  const bool sameProtocol = protocol == lastRemoteProtocol ||
                            (currentIsNEC && previousIsNEC);
  return address == lastRemoteAddress && sameProtocol;
}

void do_IR_Tick() {
  // 古いリピート信号で停止状態を復活させないよう、受信より先に期限を見る。
  updateRemoteHold();
  if (!IrReceiver.decode()) {
    return;
  }

  const uint8_t flags = IrReceiver.decodedIRData.flags;
  const uint16_t command = IrReceiver.decodedIRData.command;
  const uint16_t address = IrReceiver.decodedIRData.address;
  const decode_type_t protocol = IrReceiver.decodedIRData.protocol;
  IrReceiver.resume();

  if ((flags & IRDATA_FLAGS_WAS_OVERFLOW) ||
      (flags & IRDATA_FLAGS_PARITY_FAILED) || protocol == UNKNOWN) {
    resetToStop();
    return;
  }

  if (command == IR_STOP) {
    resetToStop();
    return;
  }

  if (flags & IRDATA_FLAGS_IS_REPEAT) {
    // 長押し中の同じ方向・同じ送信元・期限内の信号だけを受け付ける。
    if (holdActive && command == lastRemoteCommand &&
        isSameRemote(address, protocol)) {
      lastRemoteAt = millis();
    }
    return;
  }

  const DN requested = decodeRemoteDirection(command);
  if (requested == STOP_STOP) {
    return;  // 未使用ボタンでは走行時間を延長しない
  }

  Remote_Num = requested;
  holdActive = true;
  lastRemoteAt = millis();
  lastRemoteCommand = command;
  lastRemoteAddress = address;
  lastRemoteProtocol = protocol;
}

// ============================================================================
// 7. センサー読み取り
// ============================================================================
float watch() {
  digitalWrite(Trig_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(Trig_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig_PIN, LOW);

  const unsigned long echoTime = pulseIn(Echo_PIN, HIGH, ECHO_TIMEOUT_US);
  if (echoTime == 0) {
    return -1.0f;
  }
  return echoTime * 0.01657f;
}

void updateSensors() {
  const uint32_t now = millis();
  if (!sensorSampled || uint32_t(now - sensorAt) >= SENSOR_PERIOD_MS) {
    distanceCm = watch();
    distanceValid = distanceCm >= 0.0f;
    sensorAt = now;
    sensorSampled = true;
  }

  front = distanceValid && distanceCm <= REVERSE_DISTANCE_CM;
  back = digitalRead(LFSensor_bl) == REAR_ACTIVE_LEVEL ||
         digitalRead(LFSensor_br) == REAR_ACTIVE_LEVEL;
}

// ============================================================================
// 8. 障害物回避と手動操作の優先順位
// ============================================================================
void checkSensorsAndObstacles() {
  updateRemoteHold();

  // ボタンを離した場合、回避中でも停止する。
  if (!holdActive || Remote_Num == STOP_STOP) {
    resetToStop();
    return;
  }

  if (!distanceValid || (front && back)) {
    resetToStop();
    return;
  }

  const uint32_t now = millis();
  if (avoidMode != AVOID_NONE && uint32_t(now - avoidStarted) >= AUTO_MAX_MS) {
    resetToStop();
    return;
  }

  // 前方回避中：後方を監視しながら、15cmより離れるまで後退する。
  if (avoidMode == AVOID_FRONT) {
    if (back || distanceCm > RELEASE_DISTANCE_CM) {
      resetToStop();
      return;
    }
    Drive_Num = GO_BACK;
    DriveSpeed = AVOID_SPEED;
    return;
  }

  // 後方回避中：前方を監視しながら、後方検知がなくなるまで前進する。
  if (avoidMode == AVOID_REAR) {
    if (front || !back) {
      resetToStop();
      return;
    }
    Drive_Num = GO_ADVANCE;
    DriveSpeed = AVOID_SPEED;
    return;
  }

  if (front) {
    avoidMode = AVOID_FRONT;
    avoidStarted = now;
    Drive_Num = GO_BACK;
    DriveSpeed = AVOID_SPEED;
    return;
  }

  if (back) {
    avoidMode = AVOID_REAR;
    avoidStarted = now;
    Drive_Num = GO_ADVANCE;
    DriveSpeed = AVOID_SPEED;
    return;
  }

  Drive_Num = Remote_Num;
  if (Drive_Num == GO_ADVANCE) {
    DriveSpeed = ADVANCE_SPEED;
  } else if (Drive_Num == GO_BACK) {
    DriveSpeed = BACK_SPEED;
  } else {
    DriveSpeed = TURN_SPEED;
  }
}

// ============================================================================
// 9. 距離に応じたブザー（delayを使わない）
// ============================================================================
uint32_t getBuzzerPeriod(float cm) {
  if (cm <= REVERSE_DISTANCE_CM) {
    return BUZZ_FAST_MS;
  }
  if (cm >= WARNING_DISTANCE_CM) {
    return BUZZ_SLOW_MS;
  }

  const float ratio = (cm - REVERSE_DISTANCE_CM) /
                      (WARNING_DISTANCE_CM - REVERSE_DISTANCE_CM);
  return BUZZ_FAST_MS + uint32_t(ratio * (BUZZ_SLOW_MS - BUZZ_FAST_MS));
}

void warning() {
  const bool alarm = distanceValid && distanceCm <= WARNING_DISTANCE_CM;
  if (!alarm) {
    digitalWrite(BUZZ_PIN, BUZZ_ACTIVE_LEVEL == LOW ? HIGH : LOW);
    buzzerRunning = false;
    return;
  }

  const uint32_t now = millis();
  const uint32_t period = getBuzzerPeriod(distanceCm);
  if (!buzzerRunning || uint32_t(now - buzzerCycleAt) >= period) {
    buzzerCycleAt = now;
    buzzerRunning = true;
  }

  const bool on = uint32_t(now - buzzerCycleAt) < BUZZ_ON_MS;
  digitalWrite(BUZZ_PIN, on ? BUZZ_ACTIVE_LEVEL :
                            (BUZZ_ACTIVE_LEVEL == LOW ? HIGH : LOW));
}

// ============================================================================
// 10. モーター実行（逆転前に100ms停止）
// ============================================================================
void do_Drive_Tick() {
  if (Drive_Num == STOP_STOP) {
    stop_Stop();
    directionWaiting = false;
    return;
  }

  int right = 0;
  int left = 0;
  switch (Drive_Num) {
    case GO_ADVANCE: right = DriveSpeed;  left = DriveSpeed;  break;
    case GO_BACK:    right = -DriveSpeed; left = -DriveSpeed; break;
    case GO_RIGHT:   right = -DriveSpeed; left = DriveSpeed;  break;
    case GO_LEFT:    right = DriveSpeed;  left = -DriveSpeed; break;
    default:        stop_Stop(); return;
  }

  const uint32_t now = millis();
  if (directionWaiting) {
    if (uint32_t(now - directionStoppedAt) < DIRECTION_STOP_MS) {
      return;
    }
    directionWaiting = false;
  }

  const bool reversing = (appliedRight > 0 && right < 0) ||
                         (appliedRight < 0 && right > 0) ||
                         (appliedLeft > 0 && left < 0) ||
                         (appliedLeft < 0 && left > 0);
  if (reversing) {
    stop_Stop();
    directionWaiting = true;
    directionStoppedAt = now;
    return;
  }

  switch (Drive_Num) {
    case GO_ADVANCE: go_Advance(); break;
    case GO_BACK:    go_Back();    break;
    case GO_RIGHT:   go_Right();   break;
    case GO_LEFT:    go_Left();    break;
    default:        stop_Stop();  break;
  }
}

// ============================================================================
// 11. 初期化とメイン処理
// ============================================================================
void setup() {
  const uint8_t outputs[] = {
    speedPinR, RightDirectPin1, RightDirectPin2,
    speedPinL, LeftDirectPin1, LeftDirectPin2, BUZZ_PIN, Trig_PIN
  };
  for (uint8_t pin : outputs) {
    pinMode(pin, OUTPUT);
  }

  stop_Stop();
  resetToStop();
  digitalWrite(BUZZ_PIN, BUZZ_ACTIVE_LEVEL == LOW ? HIGH : LOW);
  digitalWrite(Trig_PIN, LOW);
  pinMode(LFSensor_bl, INPUT);
  pinMode(LFSensor_br, INPUT);
  pinMode(Echo_PIN, INPUT);

  headServo.attach(SERVO_PIN);
  headServo.write(90);
  delay(400);  // 起動時だけ正面合わせ。モーターは停止中。
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
}

void loop() {
  do_IR_Tick();                 // 1. 長押し信号と停止ボタン
  updateSensors();              // 2. 前方距離・後方障害物
  do_IR_Tick();                 // 3. 測距中に届いた信号も反映
  checkSensorsAndObstacles();   // 4. 停止 → 回避 → 手動操作
  warning();                   // 5. 距離に応じた警告音
  do_Drive_Tick();              // 6. モーターへ出力
}
