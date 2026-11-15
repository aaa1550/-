#pragma once
#include <stdint.h>
#define IRDATA_FLAGS_WAS_OVERFLOW 64
#define IRDATA_FLAGS_IS_REPEAT 1
#define IRDATA_FLAGS_PARITY_FAILED 4
enum decode_type_t { UNKNOWN, NEC, NEC2, SONY };
#define DISABLE_LED_FEEDBACK false
struct Receiver {
  struct {uint8_t flags=0;uint16_t command=0;uint16_t address=0;decode_type_t protocol=NEC;} decodedIRData;
  bool pending=false;
  bool decode(){return pending;}
  void resume(){pending=false;}
  void begin(int,bool){}
};
extern Receiver IrReceiver;
