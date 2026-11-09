// 単独モーターベンチ診断。障害物回避は行わない。車輪を浮かせて実施。
// 仮定: 左右各IN1/IN2/PWMのHブリッジ。OSOYOO実配線は未確認。
#include <Arduino.h>
constexpr bool WIRING_CONFIRMED=false;
constexpr int8_t L1=-1,L2=-1,LP=-1,R1=-1,R2=-1,RP=-1;
uint32_t stopAt=0,startedAt=0;bool moving=false,pending=false;char command='x';
bool ready(){int8_t pins[]={L1,L2,LP,R1,R2,RP};for(uint8_t i=0;i<6;i++){if(pins[i]<2||pins[i]>19)return false;for(uint8_t j=0;j<i;j++)if(pins[i]==pins[j])return false;}return WIRING_CONFIRMED&&(LP==5||LP==6||LP==9||LP==10)&&(RP==5||RP==6||RP==9||RP==10);}
void stop(){analogWrite(LP,0);analogWrite(RP,0);digitalWrite(L1,LOW);digitalWrite(L2,LOW);digitalWrite(R1,LOW);digitalWrite(R2,LOW);moving=false;}
void setup(){Serial.begin(115200);if(!ready()){Serial.println(F("VERIFY DRIVER AND PINS FIRST"));return;}int8_t p[]={L1,L2,LP,R1,R2,RP};for(uint8_t i=0;i<6;i++){digitalWrite(p[i],LOW);pinMode(p[i],OUTPUT);}stop();}
void loop(){if(!ready())return;uint32_t now=millis();if(Serial.available()){char c=Serial.read();if(c=='f'||c=='b'||c=='x'){stop();command=c;stopAt=now;pending=c!='x';}}if(pending&&uint32_t(now-stopAt)>=100){bool f=command=='f';digitalWrite(L1,f?HIGH:LOW);digitalWrite(L2,f?LOW:HIGH);digitalWrite(R1,f?HIGH:LOW);digitalWrite(R2,f?LOW:HIGH);analogWrite(LP,60);analogWrite(RP,60);startedAt=now;moving=true;pending=false;}if(moving&&uint32_t(now-startedAt)>=800)stop();}
