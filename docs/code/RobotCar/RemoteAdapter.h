#pragma once
#include <Arduino.h>
#include <IRremote.hpp>
#include "BoardConfig.h"
#include "RobotLogic.h"
namespace remote {
// @function codesReady
inline bool codesReady() {
 const uint16_t keys[]={board::KEY_FORWARD,board::KEY_BACKWARD,board::KEY_LEFT,board::KEY_RIGHT,board::KEY_STOP};
 for(uint8_t i=0;i<5;++i)for(uint8_t j=0;j<i;++j)if(keys[i]==keys[j])return false;
 return board::REMOTE_CODES_CONFIRMED;
}
// @endfunction
// @function decodeCommand
inline bool decodeCommand(uint16_t code,robot::Motion& out) {
 if(!codesReady())return false;
 if(code==board::KEY_FORWARD)out=robot::Motion::Forward;
 else if(code==board::KEY_BACKWARD)out=robot::Motion::Backward;
 else if(code==board::KEY_LEFT)out=robot::Motion::Left;
 else if(code==board::KEY_RIGHT)out=robot::Motion::Right;
 else if(code==board::KEY_STOP)out=robot::Motion::Stop;
 else return false;
 return true;
}
// @endfunction
// @function pollRemote
inline void pollRemote(robot::RemoteState& state,uint32_t now,const robot::Parameters& p) {
 if(!IrReceiver.decode())return;
 const auto decoded=IrReceiver.decodedIRData;
 IrReceiver.resume();
 // This adapter intentionally handles NEC/NEC2 only. Other remotes need protocol validation.
 if(!codesReady()||(decoded.protocol!=NEC&&decoded.protocol!=NEC2)||decoded.address!=board::REMOTE_ADDRESS||(decoded.flags&IRDATA_FLAGS_WAS_OVERFLOW))return;
 robot::Motion motion=robot::Motion::Stop;
 if(!decodeCommand(decoded.command,motion))return;
 bool repeat=(decoded.flags&IRDATA_FLAGS_IS_REPEAT)!=0;
 if(repeat&&(!robot::remoteFresh(state,now,p)||motion!=state.command))return;
 robot::acceptRemote(state,motion,now,repeat);
}
// @endfunction
}
