#include <cassert>
#include <cstring>
#include <iostream>
#include <Arduino.h>
#include <IRremote.hpp>
uint32_t fakeNow=0, fakeMicros=0;
uint32_t pinHighAt[32]={}, pinLowAt[32]={};
int pinWrites[32]={}, pins[32]={}, pwm[32]={};
unsigned long echoUs=0;
Receiver IrReceiver;
#include "../CompletedObstacleCar.ino"

void reset() {
  fakeNow=1000;
  memset(pwm,0,sizeof pwm);
  memset(pins,HIGH,sizeof pins);
  pins[LFSensor_bl]=HIGH; pins[LFSensor_br]=HIGH;
  requested=STOP; holdActive=false;
  appliedRight=appliedLeft=0; stopTimingActive=false;
  sensorStarted=false; buzzerActive=false;
  SR1=SR2=IR=false; RM=M=0; distanceValid=false;
  echoUs=1160; IrReceiver.pending=false;
}
void send(uint16_t command,uint8_t flags=0,uint16_t address=7,
          decode_type_t protocol=NEC) {
  IrReceiver.decodedIRData.command=command;
  IrReceiver.decodedIRData.flags=flags;
  IrReceiver.decodedIRData.address=address;
  IrReceiver.decodedIRData.protocol=protocol;
  IrReceiver.pending=true;
}
void distance(float cm) {
  sensorStarted=false;
  echoUs=static_cast<unsigned long>(cm*58.0f);
}
void tick() { lastReceivedAt=fakeNow; loop(); }
int main() {
  static_assert(speedPinR==9 && speedPinL==6 && IR_RECEIVE_PIN==4,"pins");
  static_assert(Echo_PIN==3 && Trig_PIN==10 && LFSensor_bl==0 &&
                LFSensor_br==2 && BUZZ_PIN==13,"sensor pins");
  static_assert(MOVE_SPEED==180 && TURN_SPEED==180,"original speed");
  const uint16_t commands[]={IR_ADVANCE,IR_BACK,IR_RIGHT,IR_LEFT};
  const int rights[]={180,-180,-180,180};
  const int lefts[]={180,-180,180,-180};
  for (int i=0;i<4;i++) {
    reset(); send(commands[i]); loop();
    assert(RM==i+1 && M==i+3);
    assert(appliedRight==rights[i] && appliedLeft==lefts[i]);
    fakeNow+=200; send(commands[i],IRDATA_FLAGS_IS_REPEAT,7,NEC2); loop();
    assert(holdActive && appliedRight==rights[i]);
    fakeNow+=249; loop(); assert(holdActive);
    fakeNow+=1; loop(); assert(!holdActive && pwm[9]==0 && pwm[6]==0);
    send(commands[i],IRDATA_FLAGS_IS_REPEAT); loop(); assert(!holdActive);
  }
  // 前方10cm以下：全方向で後退。100msの切替待ちも確認。
  for (auto cmd:commands) {
    reset(); send(cmd); loop(); distance(10); fakeNow+=10; tick();
    assert(SR1 && !SR2 && M==1);
    fakeNow+=100; tick();
    assert(appliedRight==-100 && appliedLeft==-100 && pwm[9]==100);
    assert(pins[BUZZ_PIN]==LOW);
    // 10cmより離れれば元の操作へ戻る（画像に回避ラッチはない）。
    distance(11); fakeNow+=1; tick(); assert(!SR1 && SR2 && M>=3);
    fakeNow+=100; tick(); assert(pwm[9]==180);
  }
  // 後方は左右のどちらでも前進、後方警告も鳴らす。
  for (int pin:{LFSensor_bl,LFSensor_br}) {
    reset(); send(IR_BACK); loop(); pins[pin]=LOW; fakeNow+=1; tick();
    assert(IR && M==2 && pwm[9]==0);
    fakeNow+=100; tick(); assert(appliedRight==100 && appliedLeft==100);
    assert(pins[BUZZ_PIN]==LOW);
    pins[pin]=HIGH; fakeNow+=1; tick(); assert(M==4);
    fakeNow+=100; tick(); assert(appliedRight==-180);
  }
  // 15cm境界、10cm境界、フラグ解除、無応答。
  reset(); distance(15); updateUltrasonicState(); assert(SR2 && !SR1);
  distance(10); updateUltrasonicState(); assert(SR1 && !SR2);
  distance(20); updateUltrasonicState(); assert(!SR1 && !SR2);
  echoUs=0; sensorStarted=false; updateUltrasonicState(); assert(!distanceValid);
  // 前後同時・超音波失敗・OK・長押し解除では回避も停止。
  reset(); send(IR_ADVANCE); distance(4); pins[LFSensor_bl]=LOW; loop();
  assert(M==0 && pwm[9]==0);
  reset(); send(IR_ADVANCE); echoUs=0; loop(); assert(M==0 && pwm[9]==0);
  reset(); distance(4); loop(); assert(M==0 && !holdActive); // 起動時
  send(IR_ADVANCE); loop(); assert(M==1 && pwm[9]==100);
  send(IR_STOP); loop(); assert(M==0 && !holdActive && pwm[9]==0);
  send(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT); loop(); assert(!holdActive);
  reset(); send(IR_ADVANCE); distance(4); loop(); fakeNow+=250; loop();
  assert(M==0 && pwm[9]==0);
  // 同じ送信元だけが長押し期限を延長する。
  reset(); send(IR_ADVANCE); loop(); fakeNow+=200;
  send(IR_ADVANCE,IRDATA_FLAGS_IS_REPEAT,8); loop();
  fakeNow+=50; loop(); assert(!holdActive);
  reset(); send(IR_ADVANCE); loop(); send(IR_ADVANCE,IRDATA_FLAGS_PARITY_FAILED);
  loop(); assert(!holdActive && pwm[9]==0);
  // ブザー：15cm=600ms、12.5cm=350ms、10cm/後方=100ms、ON35ms。
  for (float cm:{15.0f,12.5f,10.0f}) {
    reset(); SD=cm; SR1=cm<=10; SR2=!SR1;
    updateBuzzerState(); assert(pins[13]==LOW);
    fakeNow+=35; updateBuzzerState(); assert(pins[13]==HIGH);
    fakeNow=1000+(cm==15?600:cm==12.5f?350:100);
    updateBuzzerState(); assert(pins[13]==LOW);
    SR1=SR2=IR=false; updateBuzzerState(); assert(pins[13]==HIGH);
  }
  // millisの折り返しでも長押し期限を判定する。
  reset(); fakeNow=0xfffffff0U; send(IR_ADVANCE); loop();
  fakeNow+=250; loop(); assert(!holdActive && pwm[9]==0);
  std::cout<<"PASS: original controls, all directions, obstacle states, boundaries, buzzer, timeout, reversal, timer wrap\n";
}
