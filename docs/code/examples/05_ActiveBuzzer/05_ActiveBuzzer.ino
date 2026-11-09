// デジタルON/OFF型のアクティブブザー専用。受動型にはそのまま使用しない。
// tone()を使わず、IRremoteとのタイマー共有を避ける構成例。
#include <Arduino.h>
constexpr int8_t BUZZER_PIN=-1;
constexpr bool ACTIVE_HIGH=true;
void setup(){Serial.begin(115200);if(BUZZER_PIN<2||BUZZER_PIN>19){Serial.println(F("SET BUZZER_PIN FIRST"));return;}digitalWrite(BUZZER_PIN,ACTIVE_HIGH?LOW:HIGH);pinMode(BUZZER_PIN,OUTPUT);}
void loop(){if(BUZZER_PIN<2||BUZZER_PIN>19)return;bool on=millis()%400<200;digitalWrite(BUZZER_PIN,(on==ACTIVE_HIGH)?HIGH:LOW);}
