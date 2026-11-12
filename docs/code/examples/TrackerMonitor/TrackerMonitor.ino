/* 5.jpgのOSOYOO 5-Channel IR Tracker専用の読み取り診断。
 * 画像の配線: IR1=A0, IR2=A1, IR3=A2, IR4=A3, IR5=A4。
 * 車両の前進・後退は行わない。値をSerial Monitor 115200bpsへ出す。
 * D0へ後方センサーを接続している場合、電源を切って信号線を外してから使う。
 * 各チャンネルの0/1をそのまま表示し、後方障害物としては判定しない。
 */
#include <Arduino.h>
constexpr uint8_t TRACKER_PINS[]={A0,A1,A2,A3,A4};
uint32_t previous=0;
void setup() {
  // 改訂版ENA D5 / ENB D6、Model Xの方向線を停止へ。
  pinMode(5,OUTPUT);pinMode(6,OUTPUT);analogWrite(5,0);analogWrite(6,0);
  const uint8_t directions[]={12,11,7,8};
  for(uint8_t pin:directions) {pinMode(pin,OUTPUT);digitalWrite(pin,LOW);}
  pinMode(13,OUTPUT);digitalWrite(13,HIGH);
  for(uint8_t pin:TRACKER_PINS) pinMode(pin,INPUT);
  Serial.begin(115200);
  Serial.println(F("IR1(A0),IR2(A1),IR3(A2),IR4(A3),IR5(A4) / raw 0=LOW 1=HIGH"));
}
void loop() {
  const uint32_t now=millis();
  if(uint32_t(now-previous)<200U) return;
  previous=now;
  for(uint8_t i=0;i<5;++i) {
    if(i) Serial.print(',');
    Serial.print(digitalRead(TRACKER_PINS[i]));
  }
  Serial.println();
}
