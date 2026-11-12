/*
 * OSOYOO ROBOT CAR / Arduino UNO / Model X
 * 今回の指定動作:
 * リモコンで前進・後退・右旋回・左旋回。
 * 前方15cm以内で警告し、近づくほどブザー間隔を短くする。
 * 前方5cm以下で強制後退。後方検知で自動前進。
 * STOP、測定失敗、前方5cm以下＋後方同時検知では停止。
 * この1ファイルだけで使用可能。実機走行は未検証。
 */
#include <Arduino.h>

// ===== ライブラリ =====
#define IR_USE_AVR_TIMER2
#include <IRremote.hpp>  // Arduino-IRremote 4.4.1
#include <Servo.h>       // Servo 1.2.2

// ===== ピン設定（code.txtと追加配線資料を参照） =====
#define IR_RECEIVE_PIN    4
#define speedPinR         5   // 右ENA: code.txtのD9からD5へ信号線を移す
#define RightDirectPin1  12
#define RightDirectPin2  11
#define speedPinL         6
#define LeftDirectPin1    7
#define LeftDirectPin2    8
#define BUZZ_PIN         13
#define LFSensor_bl       0   // 左後方。D0と共用するSerialは使わない
#define LFSensor_br       2   // 右後方
#define Echo_PIN          3   // 配線.jpgのD2とは異なる。ここではcode.txtのD3
#define Trig_PIN         10
#define SERVO_PIN         9   // サーボ信号: code.txtのD5からD9へ移す
#define REAR_ACTIVE_LEVEL LOW
#define BUZZ_ACTIVE_LEVEL LOW

// ===== リモコンのボタン値（code.txtと同じ） =====
#define IR_ADVANCE       24
#define IR_BACK          82
#define IR_RIGHT         90
#define IR_LEFT           8
#define IR_STOP          28

// ===== 距離（cm） =====
#define WARNING_DISTANCE_CM  15.0f
#define REVERSE_DISTANCE_CM   5.0f
#define RELEASE_DISTANCE_CM  15.0f // 強制後退は15cmより離れたら終了

// ===== PWM速度（0〜255、実機で調整する初期値） =====
#define ADVANCE_SPEED       128    // 約50%
#define BACK_SPEED           77    // 約30%
#define TURN_SPEED           77    // 約30%
#define AVOID_SPEED          80    // 約31%。回避時の始動を実機で確認する

// ===== 時間（ms、ただしECHO_TIMEOUT_USはμs） =====
#define SENSOR_PERIOD_MS     60UL
#define ECHO_TIMEOUT_US   15000UL
#define DIRECTION_STOP_MS   100UL
#define AUTO_MAX_MS        3000UL  // 回避が完了しない場合は停止
#define BUZZ_SLOW_MS        600UL  // 15cm時の周期
#define BUZZ_FAST_MS        100UL  // 5cm以下時の周期
#define BUZZ_ON_MS           35UL

// UNOではServoがTimer1、IRremoteがTimer2を使用する。
// モーターPWMはそれらと競合しないD5/D6を使う。
static_assert(speedPinR==5 && speedPinL==6,
              "UNO: keep motor PWM on D5/D6 with Servo and IRremote");
static_assert(Echo_PIN!=LFSensor_bl && Echo_PIN!=LFSensor_br,
              "Echo and rear sensor pins must be different");

static_assert(REVERSE_DISTANCE_CM>0 && WARNING_DISTANCE_CM>REVERSE_DISTANCE_CM &&
              RELEASE_DISTANCE_CM>=WARNING_DISTANCE_CM,"Invalid distance thresholds");
static_assert(BUZZ_ON_MS<BUZZ_FAST_MS && BUZZ_FAST_MS<=BUZZ_SLOW_MS,
              "Invalid buzzer timing");
static_assert(ADVANCE_SPEED>=0 && ADVANCE_SPEED<=255 && BACK_SPEED>=0 && BACK_SPEED<=255 &&
              TURN_SPEED>=0 && TURN_SPEED<=255 && AVOID_SPEED>=0 && AVOID_SPEED<=255,
              "PWM speed must be between 0 and 255");

// ===== 状態 =====
enum DN { GO_ADVANCE, GO_BACK, GO_RIGHT, GO_LEFT, STOP_STOP };
enum AvoidMode { AVOID_NONE, AVOID_FRONT, AVOID_REAR };
DN Remote_Num=STOP_STOP;       // ユーザーが指定した動作
DN Drive_Num=STOP_STOP;        // センサー判定後に実行する動作
AvoidMode avoidMode=AVOID_NONE;
Servo headServo;
float distanceCm=-1.0f;
bool distanceValid=false;
bool sensorSampled=false;
int front=0;                  // 前方5cm以下なら1
int back=0;                   // 後方検知なら1
int DriveSpeed=0;
uint32_t sensorAt=0,avoidStarted=0;
bool buzzerRunning=false;
uint32_t buzzerCycleAt=0;
int appliedRight=0,appliedLeft=0;
bool directionWaiting=false;
uint32_t directionStoppedAt=0;

// ===== モーター出力 =====
void setMotors(int rightSpeed,int leftSpeed) {
  digitalWrite(RightDirectPin1,rightSpeed>0?HIGH:LOW);
  digitalWrite(RightDirectPin2,rightSpeed<0?HIGH:LOW);
  digitalWrite(LeftDirectPin1,leftSpeed>0?HIGH:LOW);
  digitalWrite(LeftDirectPin2,leftSpeed<0?HIGH:LOW);
  analogWrite(speedPinR,rightSpeed<0?-rightSpeed:rightSpeed);
  analogWrite(speedPinL,leftSpeed<0?-leftSpeed:leftSpeed);
  appliedRight=rightSpeed;appliedLeft=leftSpeed;
}
void go_Advance() { setMotors(DriveSpeed,DriveSpeed); }
void go_Back()    { setMotors(-DriveSpeed,-DriveSpeed); }
void go_Right()   { setMotors(-DriveSpeed,DriveSpeed); }
void go_Left()    { setMotors(DriveSpeed,-DriveSpeed); }
void stop_Stop() {
  analogWrite(speedPinR,0);analogWrite(speedPinL,0);
  digitalWrite(RightDirectPin1,LOW);digitalWrite(RightDirectPin2,LOW);
  digitalWrite(LeftDirectPin1,LOW);digitalWrite(LeftDirectPin2,LOW);
  appliedRight=0;appliedLeft=0;
}

// STOP、異常、回避完了時は以前の方向指令も解除する。
void resetToStop() {
  Remote_Num=STOP_STOP;Drive_Num=STOP_STOP;DriveSpeed=0;
  avoidMode=AVOID_NONE;
}

// ===== リモコン受信 =====
// 方向キーを1回押すとその動作を保持。OKで停止（code.txtと同じ操作方式）。
void do_IR_Tick() {
  if(!IrReceiver.decode()) return;
  const uint8_t flags=IrReceiver.decodedIRData.flags;
  const uint16_t command=IrReceiver.decodedIRData.command;
  if(flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    resetToStop();
  } else if(!(flags & IRDATA_FLAGS_IS_REPEAT)) {
    switch(command) {
      case IR_ADVANCE:Remote_Num=GO_ADVANCE;break;
      case IR_BACK:Remote_Num=GO_BACK;break;
      case IR_RIGHT:Remote_Num=GO_RIGHT;break;
      case IR_LEFT:Remote_Num=GO_LEFT;break;
      case IR_STOP:resetToStop();break;
      default:break; // 未使用ボタンで動作を変更しない
    }
  }
  IrReceiver.resume();
}

// ===== 超音波距離測定 =====
float watch() {
  digitalWrite(Trig_PIN,LOW);delayMicroseconds(2);
  digitalWrite(Trig_PIN,HIGH);delayMicroseconds(10);
  digitalWrite(Trig_PIN,LOW);
  const unsigned long echoTime=pulseIn(Echo_PIN,HIGH,ECHO_TIMEOUT_US);
  if(echoTime==0) return -1.0f; // 失敗を「遠い=安全」と判定しない
  return echoTime*0.01657f;    // code.txtと同じ換算係数、丸めずに返す
}
void updateSensors() {
  const uint32_t now=millis();
  if(!sensorSampled || uint32_t(now-sensorAt)>=SENSOR_PERIOD_MS) {
    distanceCm=watch();distanceValid=distanceCm>=0.0f;
    sensorAt=now;sensorSampled=true;
  }
  front=distanceValid && distanceCm<=REVERSE_DISTANCE_CM;
  back=digitalRead(LFSensor_bl)==REAR_ACTIVE_LEVEL ||
       digitalRead(LFSensor_br)==REAR_ACTIVE_LEVEL;
}

// ===== 障害物による動作の切り替え =====
void checkSensorsAndObstacles() {
  // 起動直後とOK停止中は、障害物を検知しても走り出さない。
  if(Remote_Num==STOP_STOP) {Drive_Num=STOP_STOP;DriveSpeed=0;avoidMode=AVOID_NONE;return;}
  if(!distanceValid) {resetToStop();return;}
  // 前方5cm以下で後退先もふさがっている場合は停止する。
  if(front && back) {resetToStop();return;}

  const uint32_t now=millis();
  if(avoidMode!=AVOID_NONE && uint32_t(now-avoidStarted)>=AUTO_MAX_MS) {
    resetToStop();return;
  }
  if(avoidMode==AVOID_FRONT) {
    if(back) {resetToStop();return;} // 後退中も後方を監視
    if(distanceCm>RELEASE_DISTANCE_CM) {resetToStop();return;}
    Drive_Num=GO_BACK;DriveSpeed=AVOID_SPEED;return;
  }
  if(avoidMode==AVOID_REAR) {
    if(front) {resetToStop();return;} // 前進中も前方5cmを監視
    if(!back) {resetToStop();return;}
    Drive_Num=GO_ADVANCE;DriveSpeed=AVOID_SPEED;return;
  }
  if(front) { // 5cm以下: リモコンの方向を上書きして後退
    avoidMode=AVOID_FRONT;avoidStarted=now;
    Drive_Num=GO_BACK;DriveSpeed=AVOID_SPEED;return;
  }
  if(back) { // 後方検知: リモコンの方向を上書きして前進
    avoidMode=AVOID_REAR;avoidStarted=now;
    Drive_Num=GO_ADVANCE;DriveSpeed=AVOID_SPEED;return;
  }
  // 5cm超〜15cm以内ではブザーだけ。方向はリモコン操作に従う。
  Drive_Num=Remote_Num;
  if(Drive_Num==GO_ADVANCE) DriveSpeed=ADVANCE_SPEED;
  else if(Drive_Num==GO_BACK) DriveSpeed=BACK_SPEED;
  else DriveSpeed=TURN_SPEED;
}

// ===== 距離に応じてブザーの周期を短くする =====
uint32_t getBuzzerPeriod(float cm) {
  if(cm<=REVERSE_DISTANCE_CM) return BUZZ_FAST_MS;
  if(cm>=WARNING_DISTANCE_CM) return BUZZ_SLOW_MS;
  const float ratio=(cm-REVERSE_DISTANCE_CM)/
                    (WARNING_DISTANCE_CM-REVERSE_DISTANCE_CM);
  return BUZZ_FAST_MS+uint32_t(ratio*(BUZZ_SLOW_MS-BUZZ_FAST_MS));
}
void warning() {
  const bool alarm=distanceValid && distanceCm<=WARNING_DISTANCE_CM;
  if(!alarm) {
    digitalWrite(BUZZ_PIN,BUZZ_ACTIVE_LEVEL==LOW?HIGH:LOW);
    buzzerRunning=false;return;
  }
  const uint32_t now=millis(),period=getBuzzerPeriod(distanceCm);
  if(!buzzerRunning || uint32_t(now-buzzerCycleAt)>=period) {
    buzzerCycleAt=now;buzzerRunning=true;
  }
  const bool on=uint32_t(now-buzzerCycleAt)<BUZZ_ON_MS;
  digitalWrite(BUZZ_PIN,on?BUZZ_ACTIVE_LEVEL:(BUZZ_ACTIVE_LEVEL==LOW?HIGH:LOW));
}

// ===== モーター実行 =====
void do_Drive_Tick() {
  if(Drive_Num==STOP_STOP) {stop_Stop();directionWaiting=false;return;}
  int right=0,left=0;
  switch(Drive_Num) {
    case GO_ADVANCE:right=DriveSpeed;left=DriveSpeed;break;
    case GO_BACK:right=-DriveSpeed;left=-DriveSpeed;break;
    case GO_RIGHT:right=-DriveSpeed;left=DriveSpeed;break;
    case GO_LEFT:right=DriveSpeed;left=-DriveSpeed;break;
    default:stop_Stop();return;
  }
  const uint32_t now=millis();
  if(directionWaiting) {
    if(uint32_t(now-directionStoppedAt)<DIRECTION_STOP_MS) return;
    directionWaiting=false;
  }
  const bool reverse=(appliedRight>0 && right<0)||(appliedRight<0 && right>0)||
                     (appliedLeft>0 && left<0)||(appliedLeft<0 && left>0);
  if(reverse) {
    stop_Stop();directionWaiting=true;directionStoppedAt=now;return;
  }
  switch(Drive_Num) {
    case GO_ADVANCE:go_Advance();break;
    case GO_BACK:go_Back();break;
    case GO_RIGHT:go_Right();break;
    case GO_LEFT:go_Left();break;
    default:stop_Stop();break;
  }
}

// ===== 初期化とメインループ =====
void setup() {
  const uint8_t outputs[]={speedPinR,RightDirectPin1,RightDirectPin2,
                          speedPinL,LeftDirectPin1,LeftDirectPin2,BUZZ_PIN,Trig_PIN};
  for(uint8_t pin:outputs) pinMode(pin,OUTPUT);
  stop_Stop();resetToStop();
  digitalWrite(BUZZ_PIN,BUZZ_ACTIVE_LEVEL==LOW?HIGH:LOW);
  digitalWrite(Trig_PIN,LOW);
  pinMode(LFSensor_bl,INPUT);pinMode(LFSensor_br,INPUT);pinMode(Echo_PIN,INPUT);
  headServo.attach(SERVO_PIN);headServo.write(90);
  delay(400); // 起動時の正面合わせ。モーターは停止中。
  IrReceiver.begin(IR_RECEIVE_PIN,DISABLE_LED_FEEDBACK);
}
void loop() {
  do_IR_Tick();
  updateSensors();
  do_IR_Tick(); // 超音波待機中に届いたSTOPも、モーター出力前に確認
  checkSensorsAndObstacles();
  warning();
  do_Drive_Tick();
}
