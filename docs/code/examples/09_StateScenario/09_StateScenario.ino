// センサーを模擬した制御診断。モーターI/Oは行わない。
// 同じフォルダのRobotLogic.hが必要。ZIPには同梱済み。
#include <Arduino.h>
#include "RobotLogic.h"
robot::Controller ctrl;robot::Parameters params;uint32_t last=0;
void setup(){Serial.begin(115200);robot::resetController(ctrl,0);}
void loop(){uint32_t now=millis();if(uint32_t(now-last)<100)return;last=now;robot::SensorFrame s;s.frontCm=(now%6000<2000)?10:30;s.frontValid=true;s.rearValid=true;s.rearBlocked=false;s.sampledAt=now;robot::RemoteState r;r.command=robot::Motion::Forward;r.valid=true;r.receivedAt=now;r.sequence=now/6000+1;auto o=robot::updateController(ctrl,s,r,params,now);Serial.print(F("cm="));Serial.print(s.frontCm);Serial.print(F(",phase="));Serial.print(static_cast<int>(ctrl.phase));Serial.print(F(",motion="));Serial.print(static_cast<int>(o.motion));Serial.print(F(",reason="));Serial.println(static_cast<int>(o.reason));}
