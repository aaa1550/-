#include <Arduino.h>

#define IR_USE_AVR_TIMER2
#include <IRremote.hpp>

// モーターのピン
#define speedPinR         9
#define RightDirectPin1  12
#define RightDirectPin2  11
#define speedPinL         6
#define LeftDirectPin1    7
#define LeftDirectPin2    8

// リモコン
#define IR_RECEIVE_PIN    4
#define IR_ADVANCE       24
#define IR_BACK          82
#define IR_RIGHT         90
#define IR_LEFT           8
#define IR_STOP          28

// 速度：0～255
#define MOVE_SPEED      180
#define TURN_SPEED      180
#define HOLD_TIMEOUT_MS 250UL
#define DIRECTION_STOP_MS 100UL

enum Direction { STOP, FORWARD, BACKWARD, RIGHT, LEFT };
Direction requested = STOP;
bool holdActive = false;
uint32_t lastReceivedAt = 0;
uint16_t lastCommand = 0;
uint16_t lastAddress = 0;
decode_type_t lastProtocol = UNKNOWN;
int appliedRight = 0;
int appliedLeft = 0;
bool stopTimingActive = false;
uint32_t stoppedAt = 0;

void stopMotors() {
  analogWrite(speedPinR, 0);
  analogWrite(speedPinL, 0);
  digitalWrite(RightDirectPin1, LOW);
  digitalWrite(RightDirectPin2, LOW);
  digitalWrite(LeftDirectPin1, LOW);
  digitalWrite(LeftDirectPin2, LOW);
  if (appliedRight != 0 || appliedLeft != 0) {
    stoppedAt = millis();
    stopTimingActive = true;
  }
  appliedRight = 0;
  appliedLeft = 0;
}

void cancelMovement() {
  requested = STOP;
  holdActive = false;
  stopMotors();
}

void checkHoldTimeout() {
  if (holdActive &&
      uint32_t(millis() - lastReceivedAt) >= HOLD_TIMEOUT_MS) {
    cancelMovement();
  }
}

Direction getDirection(uint16_t command) {
  switch (command) {
    case IR_ADVANCE: return FORWARD;
    case IR_BACK:    return BACKWARD;
    case IR_RIGHT:   return RIGHT;
    case IR_LEFT:    return LEFT;
    default:         return STOP;
  }
}

bool sameRemote(uint16_t address, decode_type_t protocol) {
  bool currentNEC = protocol == NEC || protocol == NEC2;
  bool previousNEC = lastProtocol == NEC || lastProtocol == NEC2;
  return address == lastAddress &&
         (protocol == lastProtocol || (currentNEC && previousNEC));
}

void readRemote() {
  checkHoldTimeout();
  if (!IrReceiver.decode()) {
    return;
  }
  uint16_t command = IrReceiver.decodedIRData.command;
  uint16_t address = IrReceiver.decodedIRData.address;
  decode_type_t protocol = IrReceiver.decodedIRData.protocol;
  uint8_t flags = IrReceiver.decodedIRData.flags;
  IrReceiver.resume();

  if (protocol == UNKNOWN ||
      (flags & IRDATA_FLAGS_WAS_OVERFLOW) ||
      (flags & IRDATA_FLAGS_PARITY_FAILED)) {
    cancelMovement();
    return;
  }
  if (command == IR_STOP) {
    cancelMovement();
    return;
  }
  // 同じリモコンの長押し信号で走行を継続
  if (flags & IRDATA_FLAGS_IS_REPEAT) {
    if (holdActive && command == lastCommand &&
        sameRemote(address, protocol)) {
      lastReceivedAt = millis();
    }
    return;
  }
  Direction direction = getDirection(command);
  if (direction == STOP) {
    return;
  }
  requested = direction;
  holdActive = true;
  lastReceivedAt = millis();
  lastCommand = command;
  lastAddress = address;
  lastProtocol = protocol;
}

void driveMotors() {
  if (!holdActive || requested == STOP) {
    stopMotors();
    return;
  }
  int right = 0;
  int left = 0;
  switch (requested) {
    case FORWARD:
      right = MOVE_SPEED;
      left = MOVE_SPEED;
      break;
    case BACKWARD:
      right = -MOVE_SPEED;
      left = -MOVE_SPEED;
      break;
    case RIGHT:
      right = -TURN_SPEED;
      left = TURN_SPEED;
      break;
    case LEFT:
      right = TURN_SPEED;
      left = -TURN_SPEED;
      break;
    default:
      stopMotors();
      return;
  }
  bool reversing =
      (appliedRight > 0 && right < 0) ||
      (appliedRight < 0 && right > 0) ||
      (appliedLeft > 0 && left < 0) ||
      (appliedLeft < 0 && left > 0);
  if (reversing) {
    stopMotors();
  }
  if (stopTimingActive) {
    if (uint32_t(millis() - stoppedAt) < DIRECTION_STOP_MS) {
      return;
    }
    stopTimingActive = false;
  }
  digitalWrite(RightDirectPin1, right > 0 ? HIGH : LOW);
  digitalWrite(RightDirectPin2, right < 0 ? HIGH : LOW);
  digitalWrite(LeftDirectPin1, left > 0 ? HIGH : LOW);
  digitalWrite(LeftDirectPin2, left < 0 ? HIGH : LOW);
  analogWrite(speedPinR, right < 0 ? -right : right);
  analogWrite(speedPinL, left < 0 ? -left : left);
  appliedRight = right;
  appliedLeft = left;
}

// ============================================================================
// 【追加】障害物検知：オンライン保存した7枚のフローチャートに対応
// サーボなし。元のリモコン処理・モーター処理・速度設定は変更しない。
// ============================================================================
#define Echo_PIN              3
#define Trig_PIN             10
#define LFSensor_bl           0
#define LFSensor_br           2
#define BUZZ_PIN             13
#define REAR_ACTIVE_LEVEL   LOW
#define BUZZ_ACTIVE_LEVEL   LOW

#define FRONT_DANGER_CM    10.0f
#define FRONT_WARNING_CM   15.0f
#define SENSOR_INTERVAL_MS   60UL
#define ECHO_TIMEOUT_US   15000UL
#define AVOID_SPEED         100   // 実機で停止に見えないよう回避出力を引き上げる
#define BUZZ_SLOW_MS        600UL
#define BUZZ_FAST_MS        100UL
#define BUZZ_ON_MS           35UL

// 図と同じ名前：距離SD、前方SR1/SR2、後方IR、リモコンRM、車両M
float SD = -1.0f;
bool SR1 = false;
bool SR2 = false;
bool IR = false;
uint8_t RM = 0;
uint8_t M = 0;
bool distanceValid = false;
bool sensorStarted = false;
uint32_t lastSensorAt = 0;
bool buzzerActive = false;
uint32_t buzzerStartedAt = 0;

// Main.png：元のsetupから追加で呼び出す初期化
void setupObstacleDetection() {
  digitalWrite(Trig_PIN, LOW);
  pinMode(Trig_PIN, OUTPUT);
  pinMode(Echo_PIN, INPUT);
  pinMode(LFSensor_bl, INPUT);
  pinMode(LFSensor_br, INPUT);
  digitalWrite(BUZZ_PIN, HIGH);
}

// 超音波センサ.png：10cm以下をSR1、10cm超～15cm以下をSR2にする。
// 図では省略されている解除処理も行い、前回のTRUEを残さない。
void updateUltrasonicState() {
  const uint32_t now = millis();
  if (sensorStarted && uint32_t(now - lastSensorAt) < SENSOR_INTERVAL_MS) {
    return;
  }
  sensorStarted = true;
  lastSensorAt = now;

  digitalWrite(Trig_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(Trig_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trig_PIN, LOW);
  const unsigned long echo = pulseIn(Echo_PIN, HIGH, ECHO_TIMEOUT_US);

  distanceValid = echo > 0;
  SD = distanceValid ? echo / 58.0f : -1.0f;
  SR1 = distanceValid && SD <= FRONT_DANGER_CM;
  SR2 = distanceValid && !SR1 && SD <= FRONT_WARNING_CM;
}

// 赤外線センサ.png：左右のどちらかがLOWなら後方障害物あり。
void updateRearInfraredState() {
  IR = digitalRead(LFSensor_bl) == REAR_ACTIVE_LEVEL ||
       digitalRead(LFSensor_br) == REAR_ACTIVE_LEVEL;
}

// 車両状態.png：元のrequestedを残し、別変数Mに最終的な動作を入れる。
void updateVehicleState() {
  // 測距中に250msを超えた場合も、元の長押し停止処理を適用する。
  checkHoldTimeout();
  switch (requested) {
    case FORWARD:  RM = 1; break;
    case BACKWARD: RM = 2; break;
    case RIGHT:    RM = 3; break;
    case LEFT:     RM = 4; break;
    default:       RM = 0; break;
  }

  // 元のOK・長押し解除を優先。前後同時検知と測距失敗も停止にする。
  if (!holdActive || RM == 0 || !distanceValid || (SR1 && IR)) {
    M = 0;
  } else if (SR1) {
    M = 1;  // 前方10cm以下：低速後退
  } else if (IR) {
    M = 2;  // 後方検知：低速前進
  } else {
    M = RM + 2;  // 1→3前進、2→4後退、3→5右、4→6左
  }
}

// 完成ブザーフロー.png：SR1またはIR、またはSR2なら警告音。
// 前方10～15cmでは、以前指定された「近いほど速く」を追加する。
void updateBuzzerState() {
  if (!(SR1 || IR || SR2)) {
    digitalWrite(BUZZ_PIN, HIGH);
    buzzerActive = false;
    return;
  }
  uint32_t period = BUZZ_FAST_MS;
  if (!SR1 && !IR && SR2) {
    const float ratio = (SD - FRONT_DANGER_CM) /
                        (FRONT_WARNING_CM - FRONT_DANGER_CM);
    period = BUZZ_FAST_MS + uint32_t(ratio * (BUZZ_SLOW_MS - BUZZ_FAST_MS));
  }
  const uint32_t now = millis();
  if (!buzzerActive || uint32_t(now - buzzerStartedAt) >= period) {
    buzzerActive = true;
    buzzerStartedAt = now;
  }
  const bool on = uint32_t(now - buzzerStartedAt) < BUZZ_ON_MS;
  digitalWrite(BUZZ_PIN, on ? BUZZ_ACTIVE_LEVEL : HIGH);
}

// 行動制御.png：M=1/2だけ追加出力。逆転時の100ms停止は元と同じ。
// M=3～6はfalseを返し、元のdriveMotors()にそのまま任せる。
bool applyObstacleMovement() {
  if (M == 0) {
    stopMotors();
    return true;
  }
  if (M != 1 && M != 2) {
    return false;
  }
  const int speed = M == 1 ? -AVOID_SPEED : AVOID_SPEED;
  const bool reversing =
      (appliedRight > 0 && speed < 0) ||
      (appliedRight < 0 && speed > 0) ||
      (appliedLeft > 0 && speed < 0) ||
      (appliedLeft < 0 && speed > 0);
  if (reversing) {
    stopMotors();
  }
  if (stopTimingActive) {
    if (uint32_t(millis() - stoppedAt) < DIRECTION_STOP_MS) {
      return true;
    }
    stopTimingActive = false;
  }
  digitalWrite(RightDirectPin1, speed > 0 ? HIGH : LOW);
  digitalWrite(RightDirectPin2, speed < 0 ? HIGH : LOW);
  digitalWrite(LeftDirectPin1, speed > 0 ? HIGH : LOW);
  digitalWrite(LeftDirectPin2, speed < 0 ? HIGH : LOW);
  analogWrite(speedPinR, AVOID_SPEED);
  analogWrite(speedPinL, AVOID_SPEED);
  appliedRight = speed;
  appliedLeft = speed;
  return true;
}


void setup() {
  pinMode(speedPinR, OUTPUT);
  pinMode(RightDirectPin1, OUTPUT);
  pinMode(RightDirectPin2, OUTPUT);
  pinMode(speedPinL, OUTPUT);
  pinMode(LeftDirectPin1, OUTPUT);
  pinMode(LeftDirectPin2, OUTPUT);
  // 元の回路のブザーをOFF（LOWで鳴る想定）
  digitalWrite(13, HIGH);
  pinMode(13, OUTPUT);
  stopMotors();
  IrReceiver.begin(IR_RECEIVE_PIN, DISABLE_LED_FEEDBACK);
  setupObstacleDetection();  // 【追加】センサー初期化
}

void loop() {
  readRemote();
  checkHoldTimeout();
  // 【追加】Main.pngと同じ順序で状態を更新する
  updateUltrasonicState();
  updateRearInfraredState();
  updateVehicleState();
  updateBuzzerState();
  if (applyObstacleMovement()) {
    return;  // 障害物による後退・前進、または停止
  }
  driveMotors();
}

