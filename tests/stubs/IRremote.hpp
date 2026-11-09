#pragma once
#include "Arduino.h"
#define IRDATA_FLAGS_IS_REPEAT 1
#define IRDATA_FLAGS_WAS_OVERFLOW 64
#define DISABLE_LED_FEEDBACK false
enum decode_type_t {UNKNOWN,NEC,NEC2};
struct IRData {decode_type_t protocol=NEC;uint16_t address=0;uint16_t command=0;uint8_t flags=0;};
struct MockReceiver {IRData decodedIRData;void begin(int,bool){}bool decode(){return false;}void resume(){}void printIRResultShort(MockSerial*){}};
static MockReceiver IrReceiver;
