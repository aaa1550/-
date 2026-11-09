// HC-SR04互換のトリガ/Echo方式を仮定。実センサーの仕様確認が必要。
#include <Arduino.h>
constexpr int8_t TRIG=-1,ECHO=-1; // 配線確認後に設定。D0/D1は使用しない。
uint32_t lastRead=0;
bool ready(){return TRIG>=2&&TRIG<=19&&ECHO>=2&&ECHO<=19&&TRIG!=ECHO;}
void setup(){Serial.begin(115200);if(!ready()){Serial.println(F("SET TRIG/ECHO FIRST"));return;}digitalWrite(TRIG,LOW);pinMode(TRIG,OUTPUT);pinMode(ECHO,INPUT);}
void loop(){if(!ready())return;uint32_t now=millis();if(uint32_t(now-lastRead)<60)return;lastRead=now;digitalWrite(TRIG,LOW);delayMicroseconds(2);digitalWrite(TRIG,HIGH);delayMicroseconds(10);digitalWrite(TRIG,LOW);unsigned long t=pulseIn(ECHO,HIGH,15000UL);if(t==0)Serial.println(F("INVALID: timeout"));else{Serial.print(F("cm="));Serial.println(t*0.0343f/2.0f);}}
