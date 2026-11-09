// Arduino-IRremote 4.x が必要。モーターを駆動せず実リモコンの値を採取。
#include <Arduino.h>
#include <IRremote.hpp>
constexpr int8_t RECEIVE_PIN=-1;
void setup(){Serial.begin(115200);if(RECEIVE_PIN<2||RECEIVE_PIN>19){Serial.println(F("SET RECEIVE_PIN FIRST"));return;}IrReceiver.begin(RECEIVE_PIN,DISABLE_LED_FEEDBACK);}
void loop(){if(RECEIVE_PIN<2||RECEIVE_PIN>19)return;if(!IrReceiver.decode())return;IrReceiver.printIRResultShort(&Serial);Serial.print(F("address=0x"));Serial.print(IrReceiver.decodedIRData.address,HEX);Serial.print(F(",command=0x"));Serial.print(IrReceiver.decodedIRData.command,HEX);Serial.print(F(",repeat="));Serial.println((IrReceiver.decodedIRData.flags&IRDATA_FLAGS_IS_REPEAT)!=0);IrReceiver.resume();}
