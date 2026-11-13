#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <initializer_list>
#include "stubs/Arduino.h"
#include "stubs/IRremote.hpp"
uint32_t fakeNow=0, fakeMicros=0;
uint32_t pinHighAt[32]={}, pinLowAt[32]={};
int pinWrites[32]={};
int pins[32]={},pwm[32]={};
unsigned long echoUs=2000;
Receiver IrReceiver;
// 模擬I/Oに差し替えて、本番の単一.inoそのものをテストする。
#include "../../docs/code/RemoteObstacleCar/RemoteObstacleCar.ino"
void sample(float cm,bool rear=false) {
 distanceCm=cm;distanceValid=cm>=0;
 front=distanceValid&&cm<=REVERSE_DISTANCE_CM;back=rear;
}
void key(uint16_t cmd,uint8_t flags=0) {
 IrReceiver.decodedIRData.command=cmd;IrReceiver.decodedIRData.flags=flags;
 IrReceiver.pending=true;do_IR_Tick();
}
void fresh() {
 fakeNow=0;fakeMicros=0;servoStarted=false;lastServoAt=0;resetToStop();sensorSampled=false;
 IrReceiver.decodedIRData.protocol=NEC;IrReceiver.decodedIRData.address=0;
 buzzerRunning=false;directionWaiting=false;
 for(int i=0;i<32;i++){pins[i]=HIGH;pwm[i]=0;pinWrites[i]=0;pinHighAt[i]=0;pinLowAt[i]=0;}
 stop_Stop();sample(30);
}
int main(){
 fresh();sample(4);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 fresh();key(IR_ADVANCE);checkSensorsAndObstacles();do_Drive_Tick();
 assert(Drive_Num==GO_ADVANCE&&pwm[speedPinR]==ADVANCE_SPEED);
 // 15cmと5cmの境界: 15cmでは警告のみ、5cm以下で後退。
 sample(15.01f);checkSensorsAndObstacles();warning();assert(pins[BUZZ_PIN]==HIGH);
 sample(15.0f);checkSensorsAndObstacles();warning();assert(Drive_Num==GO_ADVANCE&&pins[BUZZ_PIN]==LOW);
 sample(5.01f);checkSensorsAndObstacles();assert(Drive_Num==GO_ADVANCE);
 sample(5.0f);checkSensorsAndObstacles();assert(Drive_Num==GO_BACK&&avoidMode==AVOID_FRONT);
 fakeNow=10;do_Drive_Tick();assert(pwm[speedPinR]==0&&directionWaiting);
 fakeNow=109;do_Drive_Tick();assert(pwm[speedPinR]==0);
 fakeNow=110;do_Drive_Tick();assert(pwm[speedPinR]==AVOID_SPEED&&pins[RightDirectPin1]==LOW&&pins[RightDirectPin2]==HIGH);
 sample(10);checkSensorsAndObstacles();assert(Drive_Num==GO_BACK);
 sample(15);checkSensorsAndObstacles();assert(Drive_Num==GO_BACK);
 sample(15.01f);checkSensorsAndObstacles();do_Drive_Tick();assert(Drive_Num==STOP_STOP&&Remote_Num==STOP_STOP&&pwm[speedPinR]==0);
 // 自動後退中のOKは即座に停止指令へ。リピートで再始動しない。
 fresh();key(IR_ADVANCE);sample(4);checkSensorsAndObstacles();assert(Drive_Num==GO_BACK);
 key(IR_STOP);checkSensorsAndObstacles();do_Drive_Tick();assert(Drive_Num==STOP_STOP&&avoidMode==AVOID_NONE);
 key(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 // 後方検知で前進、解消したら停止。前後が同時にふさがれば停止。
 fresh();key(IR_BACK);sample(30,true);checkSensorsAndObstacles();assert(Drive_Num==GO_ADVANCE&&avoidMode==AVOID_REAR);
 sample(30,false);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 key(IR_BACK);sample(5,true);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 key(IR_BACK);sample(10,true);checkSensorsAndObstacles();assert(Drive_Num==GO_ADVANCE);
 sample(5,true);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 // 手動旋回と長押し期限。
 fresh();key(IR_RIGHT);checkSensorsAndObstacles();do_Drive_Tick();assert(pins[RightDirectPin2]==HIGH&&pins[LeftDirectPin1]==HIGH);
 key(IR_LEFT);checkSensorsAndObstacles();do_Drive_Tick();assert(directionWaiting);
 fakeNow=DIRECTION_STOP_MS;do_Drive_Tick();assert(pins[RightDirectPin1]==HIGH&&pins[LeftDirectPin2]==HIGH);
 key(999);checkSensorsAndObstacles();assert(Drive_Num==GO_LEFT);
 fakeNow=10000;checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 // 無応答は安全な距離に化けず停止。オーバーフローも停止。
 sample(-1);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 key(IR_ADVANCE);sample(30);checkSensorsAndObstacles();key(IR_ADVANCE,IRDATA_FLAGS_WAS_OVERFLOW);assert(Remote_Num==STOP_STOP);
 // ブザー周期は距離に応じて単調に短くなる。
 assert(getBuzzerPeriod(15)==600&&getBuzzerPeriod(10)==350&&getBuzzerPeriod(5)==100&&getBuzzerPeriod(0)==100);
 for(int cm=6;cm<=15;cm++)assert(getBuzzerPeriod(cm)>=getBuzzerPeriod(cm-1));
 fresh();sample(15);warning();assert(pins[BUZZ_PIN]==LOW);
 fakeNow=35;warning();assert(pins[BUZZ_PIN]==HIGH);
 fakeNow=599;warning();assert(pins[BUZZ_PIN]==HIGH);
 fakeNow=600;warning();assert(pins[BUZZ_PIN]==LOW);
 sample(5);fakeNow=635;warning();assert(pins[BUZZ_PIN]==HIGH);
 fakeNow=700;warning();assert(pins[BUZZ_PIN]==LOW);
 sample(16);warning();assert(pins[BUZZ_PIN]==HIGH&&!buzzerRunning);
 // 測距処理と後方左右いずれかLOWの判定。
 fresh();echoUs=0;updateSensors();assert(!distanceValid);
 fakeNow=60;echoUs=302;updateSensors();assert(distanceValid&&distanceCm>5&&distanceCm<15&&!front);
 fakeNow=120;echoUs=301;pins[LFSensor_br]=LOW;updateSensors();assert(front&&back);
 // 自動回避の上限・時計の折り返し。
 fresh();fakeNow=0xffffff00U;key(IR_ADVANCE);sample(4);checkSensorsAndObstacles();
 for(uint32_t elapsed=100;elapsed<=AUTO_MAX_MS;elapsed+=100){
 fakeNow=uint32_t(0xffffff00U+elapsed);key(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT);checkSensorsAndObstacles();
 if(elapsed<AUTO_MAX_MS) assert(Drive_Num==GO_BACK);
 }
 assert(Drive_Num==STOP_STOP);
 fresh();fakeNow=0xfffffff0U;sample(5);warning();assert(pins[BUZZ_PIN]==LOW);
 fakeNow=uint32_t(0xfffffff0U+100U);warning();assert(pins[BUZZ_PIN]==LOW);
 // 4方向は長押しをやめると期限ちょうどで停止する。
 for(uint16_t cmd:{IR_ADVANCE,IR_BACK,IR_LEFT,IR_RIGHT}) {
  fresh();key(cmd);checkSensorsAndObstacles();do_Drive_Tick();
  assert(Drive_Num!=STOP_STOP);
  fakeNow=HOLD_TIMEOUT_MS-1;checkSensorsAndObstacles();assert(Drive_Num!=STOP_STOP);
  fakeNow=HOLD_TIMEOUT_MS;checkSensorsAndObstacles();do_Drive_Tick();
  assert(Drive_Num==STOP_STOP&&pwm[speedPinR]==0&&pwm[speedPinL]==0&&!holdActive);
 }
 // 繰り返し受信中は継続し、NEC→NEC2の変化も受け付ける。
 fresh();key(IR_ADVANCE);
 for(uint32_t t=100;t<=1000;t+=100) {
  fakeNow=t;IrReceiver.decodedIRData.protocol=NEC2;
  key(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT);checkSensorsAndObstacles();
  assert(Drive_Num==GO_ADVANCE);
 }
 fakeNow=1250;checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 key(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 key(IR_ADVANCE);checkSensorsAndObstacles();assert(Drive_Num==GO_ADVANCE);
 // 期限切れ時に届いたリピートでも復活しない。
 fresh();key(IR_ADVANCE);fakeNow=250;key(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT);
 checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 // 別送信元、別プロトコル、別キー、未知キーでは期限を延長しない。
 for(int test=0;test<4;test++) {
  fresh();key(IR_ADVANCE);fakeNow=200;
  if(test==0) IrReceiver.decodedIRData.address=1;
  if(test==1) IrReceiver.decodedIRData.protocol=SONY;
  key(test==2?IR_BACK:test==3?999:IR_ADVANCE,test==3?0:IRDATA_FLAGS_IS_REPEAT);
  fakeNow=250;checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 }
 // 自動回避中も、離すと停止。繰り返しで再始動しない。
 for(bool rear:{false,true}) {
  fresh();key(IR_ADVANCE);sample(rear?30:4,rear);
  checkSensorsAndObstacles();assert(avoidMode!=AVOID_NONE);
  fakeNow=250;checkSensorsAndObstacles();do_Drive_Tick();
  assert(Drive_Num==STOP_STOP&&pwm[speedPinR]==0);
  key(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 }
 fresh();key(IR_ADVANCE);key(IR_ADVANCE,IRDATA_FLAGS_PARITY_FAILED);
 assert(!holdActive&&Remote_Num==STOP_STOP);
 // 長押し期限はmillisの折り返しでも正しく動く。
 fresh();fakeNow=0xffffff80U;key(IR_RIGHT);
 fakeNow=uint32_t(0xffffff80U+249U);checkSensorsAndObstacles();assert(Drive_Num==GO_RIGHT);
 fakeNow=uint32_t(0xffffff80U+250U);checkSensorsAndObstacles();assert(Drive_Num==STOP_STOP);
 // サーボは元のD5に約1.5ms、目標周期20msのパルスを出す。
 fresh();pwm[speedPinR]=128;pwm[speedPinL]=77;fakeMicros=10000;keepHeadForward();
 assert(pins[SERVO_PIN]==LOW&&pinWrites[SERVO_PIN]==2);
 assert(uint32_t(pinLowAt[SERVO_PIN]-pinHighAt[SERVO_PIN])>=SERVO_CENTER_US);
 assert(uint32_t(pinLowAt[SERVO_PIN]-pinHighAt[SERVO_PIN])<=SERVO_CENTER_US+4);
 keepHeadForward();assert(pinWrites[SERVO_PIN]==2);
 fakeMicros=10000+SERVO_PERIOD_US;keepHeadForward();assert(pinWrites[SERVO_PIN]==4);
 assert(pwm[speedPinR]==128&&pwm[speedPinL]==77); // パルス処理は速度PWMへ書かない
 fresh();fakeMicros=0xfffffff0U;keepHeadForward();
 assert(uint32_t(pinLowAt[SERVO_PIN]-pinHighAt[SERVO_PIN])>=SERVO_CENTER_US);
 fakeMicros=uint32_t(0xfffffff0U+SERVO_PERIOD_US);keepHeadForward();
 assert(pinWrites[SERVO_PIN]==4);
 // センサーを読む前後でサーボを動かしても、期限切れは走行を停止する。
 fresh();key(IR_ADVANCE);fakeMicros=5000;keepHeadForward();
 fakeNow=HOLD_TIMEOUT_MS;sample(30);checkSensorsAndObstacles();do_Drive_Tick();
 assert(!holdActive&&pwm[speedPinR]==0);
 puts("PASS: original pin layout, software servo pulse width/period/wrap and PWM isolation; hold/release in all directions, valid repeats, stale repeat rejection, NEC/NEC2, sender matching, parity failure, release during avoidance; real sketch logic, 15/5cm boundaries, forced reverse, rear advance, progressive buzzer, STOP, simultaneous obstacles, motor reversal pause, sensor failure, remote hold, timeout and timer wrap");
}
