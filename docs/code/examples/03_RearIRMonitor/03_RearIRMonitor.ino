// 後方デジタル赤外線センサー診断。断線検知を保証するコードではありません。
#include <Arduino.h>
constexpr int8_t REAR_PIN=-1;
constexpr bool ACTIVE_LOW=true; // 障害物の有無と実出力レベルを照合して決定。
uint32_t lastReport=0;
void setup(){Serial.begin(115200);if(REAR_PIN<2||REAR_PIN>19){Serial.println(F("SET REAR_PIN FIRST"));return;}pinMode(REAR_PIN,INPUT_PULLUP);}
void loop(){if(REAR_PIN<2||REAR_PIN>19)return;uint32_t now=millis();if(uint32_t(now-lastReport)<100)return;lastReport=now;bool high=digitalRead(REAR_PIN)==HIGH;bool blocked=ACTIVE_LOW?!high:high;Serial.print(F("raw="));Serial.print(high);Serial.print(F(",blocked="));Serial.print(blocked);Serial.print(F(",M="));Serial.println(blocked?2:3);}
