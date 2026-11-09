// 入力診断のみ。モーターは駆動しません。115200bpsで w/s/a/d/x を送信。
#include <Arduino.h>
void setup(){Serial.begin(115200);Serial.println(F("w:forward s:backward a:left d:right x:STOP"));}
void loop(){if(!Serial.available())return;char c=Serial.read();switch(c){case 'w':Serial.println(F("FORWARD"));break;case 's':Serial.println(F("BACKWARD"));break;case 'a':Serial.println(F("LEFT"));break;case 'd':Serial.println(F("RIGHT"));break;case 'x':Serial.println(F("STOP"));break;default:break;}}
