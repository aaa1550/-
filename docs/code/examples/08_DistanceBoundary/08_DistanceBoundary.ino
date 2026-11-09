// 固定入力で5cm/15cmの境界判定を比較する。実機I/Oは使用しない。
#include <Arduino.h>
void setup(){Serial.begin(115200);float samples[]={4.9f,5.0f,5.1f,14.9f,15.0f,15.1f};for(uint8_t i=0;i<6;i++){float c=samples[i];Serial.print(c,1);Serial.print(F(",<=5:"));Serial.print(c<=5);Serial.print(F(",<5:"));Serial.print(c<5);Serial.print(F(",<=15:"));Serial.println(c<=15);}}
void loop(){}
