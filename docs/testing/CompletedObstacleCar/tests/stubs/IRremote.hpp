#pragma once
#include <stdint.h>
#define IRDATA_FLAGS_WAS_OVERFLOW 64
#define IRDATA_FLAGS_IS_REPEAT 1
#define IRDATA_FLAGS_PARITY_FAILED 4
#define DISABLE_LED_FEEDBACK false
enum decode_type_t {UNKNOWN,NEC,NEC2,SONY};
struct Receiver {
 struct {uint8_t flags=0;uint16_t command=0,address=7;decode_type_t protocol=NEC;} decodedIRData;
 bool pending=false;
 int decodeCalls=0,resumeCalls=0,beginCalls=0,beginPin=-1;
 bool feedback=true;
 bool decode(){++decodeCalls;return pending;}
 void resume(){++resumeCalls;pending=false;}
 void begin(int p,bool f){++beginCalls;beginPin=p;feedback=f;}
};
extern Receiver IrReceiver;
