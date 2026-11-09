// UNO標準の内蔵LEDによる周期実行の確認。delay()でループを止めない。
#include <Arduino.h>
uint32_t lastBlink=0,lastPrint=0;bool led=false;
void setup(){Serial.begin(115200);pinMode(LED_BUILTIN,OUTPUT);}
void loop(){uint32_t now=millis();if(uint32_t(now-lastBlink)>=250){lastBlink=now;led=!led;digitalWrite(LED_BUILTIN,led?HIGH:LOW);}if(uint32_t(now-lastPrint)>=1000){lastPrint=now;Serial.println(now);}}
