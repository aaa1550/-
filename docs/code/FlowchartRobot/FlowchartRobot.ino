/* OSOYOO Model No.2019012400 / UNO / Model X
 * code.txtを参考に、提供フローチャートの関数順・速度を実装。
 * 必須の配線変更: 右ENA D9→D5、サーボ信号 D5→D9。
 * ServoがTimer1を使うため、D9/D10でモーターPWMは使わない。
 * 同じフォルダのFlowchartLogic.hとBoardPins.hが必要。実機走行は未検証。
 */
#include <Arduino.h>
#define IR_USE_AVR_TIMER2
#include <IRremote.hpp>  // Arduino-IRremote 4.x
#include <Servo.h>
#include "FlowchartLogic.h"

#include "BoardPins.h"
constexpr uint16_t IR_ADVANCE=24,IR_BACK=82,IR_RIGHT=90,IR_LEFT=8,IR_STOP=28;
constexpr unsigned long REMOTE_TIMEOUT_MS=350;
constexpr unsigned long SENSOR_PERIOD_MS=60;

Servo headServo;
flowcar::Controller controller;
flowcar::Sensors sensors={flowcar::INVALID,false};
flowcar::Output drive={0,0,false,0};
flowcar::Remote RM=flowcar::STOP;
uint8_t M=0;                       // 車両状態（設計書参照）
uint32_t remoteAt=0,sequence=0,lastSensorAt=0;
bool remoteSeen=false,sensorSeen=false;
bool buzzerWasActive=false;
uint32_t buzzerStarted=0;
int8_t appliedRight=0,appliedLeft=0;

// リモコン状態: 未受信/STOP=0、上=1、下=2、右=3、左=4。
void do_IR_Tick() {
  if(!IrReceiver.decode()) return;
  const uint8_t flags=IrReceiver.decodedIRData.flags;
  const uint16_t key=IrReceiver.decodedIRData.command;
  if(flags & IRDATA_FLAGS_WAS_OVERFLOW) {
    remoteSeen=false;RM=flowcar::STOP;
  } else if(flags & IRDATA_FLAGS_IS_REPEAT) {
    if(remoteSeen) remoteAt=millis(); // リピートを新しい操作として扱わない
  } else {
    flowcar::Remote next=flowcar::STOP;bool known=true;
    switch(key) {
      case IR_ADVANCE:next=flowcar::FORWARD;break;
      case IR_BACK:next=flowcar::BACKWARD;break;
      case IR_RIGHT:next=flowcar::RIGHT;break;
      case IR_LEFT:next=flowcar::LEFT;break;
      case IR_STOP:next=flowcar::STOP;break;
      default:known=false;break;
    }
    if(known) {RM=next;remoteSeen=true;remoteAt=millis();++sequence;}
  }
  IrReceiver.resume();
}

// 超音波センサの状態: 5cm以下、5cm超15cm以下、安全、計測失敗。
unsigned long watch() {
  digitalWrite(Trig_PIN,LOW);delayMicroseconds(2);
  digitalWrite(Trig_PIN,HIGH);delayMicroseconds(10);
  digitalWrite(Trig_PIN,LOW);
  return pulseIn(Echo_PIN,HIGH,15000UL);
}
void updateUltrasonicState() { sensors.front=flowcar::classifyFront(watch()); }
// 左右いずれかLOWなら後方検知。Mへ直接代入せず車両状態で統合する。
void updateRearIRState() {
  sensors.rear=digitalRead(LFSensor_bl)==LOW||digitalRead(LFSensor_br)==LOW;
}
void updateVehicleState() {
  const uint32_t now=millis();
  const bool linkAlive=remoteSeen&&uint32_t(now-remoteAt)<=REMOTE_TIMEOUT_MS;
  drive=flowcar::update(controller,sensors,RM,linkAlive,sequence,now);
  M=drive.m;
}
// ブザー状態: 50ms ON/OFFを2回、600ms周期。delayで制御を止めない。
void warning() {
  const uint32_t now=millis();
  if(drive.buzzer&&!buzzerWasActive) buzzerStarted=now;
  buzzerWasActive=drive.buzzer;
  const uint32_t t=uint32_t(now-buzzerStarted)%600UL;
  const bool on=drive.buzzer&&(t<50UL||(t>=100UL&&t<150UL));
  digitalWrite(BUZZ_PIN,on?LOW:HIGH);
}
void setMotor(uint8_t in1,uint8_t in2,uint8_t pwm,int8_t percent) {
  digitalWrite(in1,percent>0?HIGH:LOW);
  digitalWrite(in2,percent<0?HIGH:LOW);
  const uint8_t magnitude=percent<0?-percent:percent;
  analogWrite(pwm,(uint16_t(magnitude)*255U+50U)/100U);
}
void stop_Stop() {
  analogWrite(speedPinR,0);analogWrite(speedPinL,0);
  digitalWrite(RightDirectPin1,LOW);digitalWrite(RightDirectPin2,LOW);
  digitalWrite(LeftDirectPin1,LOW);digitalWrite(LeftDirectPin2,LOW);
  appliedRight=0;appliedLeft=0;
}
void do_Drive_Tick() {
  if(drive.right==0&&drive.left==0) {stop_Stop();return;}
  // 出力方向が変わる場合は、先に左右のPWMを0にする。
  if((appliedRight>0&&drive.right<0)||(appliedRight<0&&drive.right>0)||
     (appliedLeft>0&&drive.left<0)||(appliedLeft<0&&drive.left>0)) stop_Stop();
  setMotor(RightDirectPin1,RightDirectPin2,speedPinR,drive.right);
  setMotor(LeftDirectPin1,LeftDirectPin2,speedPinL,drive.left);
  appliedRight=drive.right;appliedLeft=drive.left;
}
void setup() {
  const uint8_t outputs[]={RightDirectPin1,RightDirectPin2,speedPinR,
      LeftDirectPin1,LeftDirectPin2,speedPinL,BUZZ_PIN,Trig_PIN};
  for(uint8_t pin:outputs) pinMode(pin,OUTPUT);
  stop_Stop();digitalWrite(BUZZ_PIN,HIGH);digitalWrite(Trig_PIN,LOW);
  pinMode(LFSensor_bl,INPUT);pinMode(LFSensor_br,INPUT);pinMode(Echo_PIN,INPUT);
  headServo.attach(SERVO_PIN);headServo.write(90);
  delay(400); // 起動時の正面合わせだけ。モーター停止中。
  IrReceiver.begin(IR_RECEIVE_PIN,DISABLE_LED_FEEDBACK);
}
void loop() {
  do_IR_Tick();                                      // 1. リモコン状態
  const uint32_t now=millis();
  if(!sensorSeen||uint32_t(now-lastSensorAt)>=SENSOR_PERIOD_MS) {
    updateUltrasonicState();                         // 2. 超音波センサ状態
    sensorSeen=true;lastSensorAt=now;
  }
  updateRearIRState();                               // 3. 赤外線センサ状態
  do_IR_Tick(); // pulseIn待機中に届いたSTOPを、出力前にも取り込む
  updateVehicleState();                              // 4. 車両状態
  warning();                                         // 5. ブザー状態
  do_Drive_Tick();                                    // 6. 行動状態
}
