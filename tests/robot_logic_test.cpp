#include <assert.h>
#include <stdio.h>
#include "RobotLogic.h"
using namespace robot;
SensorFrame sensor(float cm,bool rear,uint32_t now){SensorFrame s;s.frontCm=cm;s.frontValid=true;s.rearValid=true;s.rearBlocked=rear;s.sampledAt=now;return s;}
RemoteState remote(Motion m,uint32_t now,uint32_t seq=1){RemoteState r;r.command=m;r.valid=true;r.receivedAt=now;r.sequence=seq;return r;}
int main(){Parameters p;
assert(validParameters(p));assert(elapsed(10,0xfffffff0UL,26));assert(!elapsed(10,0xfffffff0UL,27));
assert(clampPwm(-1)==0&&clampPwm(300)==255);assert(!validDistance(NAN));assert(!validDistance(0));assert(validDistance(200));assert(!validDistance(201));assert(isnan(pulseToCm(0)));assert(fabs(pulseToCm(1000)-17.15f)<.01f);
assert(frontEmergency(5,p));p.emergencyInclusive=false;assert(!frontEmergency(5,p));p.emergencyInclusive=true;assert(frontWarning(15,p));assert(!frontWarning(15.1f,p));
assert(rearFromLevel(false,true));assert(!rearFromLevel(true,true));assert(legacyRearState(true)==2);assert(legacyRearState(false)==3);assert(median3(4,10,3)==4);assert(isnan(median3(NAN,1,2)));
assert(wheelsFor(Motion::Forward,100).left==100);assert(wheelsFor(Motion::Left,100).right==100);assert(wheelsFor(Motion::Left,100).left==-100);assert(wheelsFor(Motion::Stop,255).right==0);
Controller c;auto out=updateController(c,sensor(30,false,0),remote(Motion::Forward,0),p,0);assert(out.motion==Motion::Forward&&out.pwm==p.normalPwm);
resetController(c,0);out=updateController(c,sensor(5,false,0),remote(Motion::Backward,0),p,0);assert(out.reason==Reason::FrontEmergency&&out.motion==Motion::Stop);
resetController(c,0);out=updateController(c,sensor(10,true,0),remote(Motion::Forward,0),p,0);assert(out.reason==Reason::BothBlocked);
resetController(c,0);auto bad=sensor(20,false,0);bad.frontValid=false;out=updateController(c,bad,remote(Motion::Forward,0),p,0);assert(out.reason==Reason::SensorFault);
resetController(c,0);out=updateController(c,sensor(30,false,0),remote(Motion::Forward,200),p,200);assert(out.reason==Reason::SensorFault);
resetController(c,0);out=updateController(c,sensor(30,false,350),remote(Motion::Forward,0),p,350);assert(out.reason==Reason::RemoteLost);
// Forward avoidance: stop first, then reverse, then hold STOP at stable clearance.
resetController(c,0);out=updateController(c,sensor(10,false,0),remote(Motion::Forward,0),p,0);assert(out.motion==Motion::Stop&&c.phase==Phase::StopBeforeEscape);
out=updateController(c,sensor(10,false,99),remote(Motion::Forward,99),p,99);assert(out.motion==Motion::Stop);
out=updateController(c,sensor(10,false,100),remote(Motion::Forward,100),p,100);assert(out.motion==Motion::Backward&&out.pwm==p.escapePwm);
out=updateController(c,sensor(25,false,200),remote(Motion::Forward,200),p,200);assert(out.motion==Motion::Stop);
out=updateController(c,sensor(25,false,600),remote(Motion::Forward,600),p,600);assert(c.phase==Phase::AwaitNewCommand);
out=updateController(c,sensor(25,false,601),remote(Motion::Forward,601),p,601);assert(out.motion==Motion::Stop);
out=updateController(c,sensor(25,false,602),remote(Motion::Forward,602,2),p,602);assert(out.motion==Motion::Forward);
// Stop command interrupts automatic avoidance.
resetController(c,0);updateController(c,sensor(10,false,0),remote(Motion::Forward,0),p,0);out=updateController(c,sensor(10,false,101),remote(Motion::Stop,101,2),p,101);assert(out.motion==Motion::Stop&&c.phase==Phase::Idle);
// Rear avoidance never enters a blocked front direction.
resetController(c,0);updateController(c,sensor(30,true,0),remote(Motion::Backward,0),p,0);out=updateController(c,sensor(30,true,100),remote(Motion::Backward,100),p,100);assert(out.motion==Motion::Forward);
out=updateController(c,sensor(10,true,101),remote(Motion::Backward,101),p,101);assert(out.motion==Motion::Stop&&c.phase==Phase::LatchedStop);
// Timeout latches and is not cleared by just holding an old forward key.
resetController(c,0);updateController(c,sensor(10,false,0),remote(Motion::Forward,0),p,0);updateController(c,sensor(10,false,100),remote(Motion::Forward,100),p,100);out=updateController(c,sensor(10,false,1600),remote(Motion::Forward,1600),p,1600);assert(out.reason==Reason::EscapeTimeout);
out=updateController(c,sensor(30,false,1601),remote(Motion::Forward,1601,2),p,1601);assert(out.motion==Motion::Stop);
updateController(c,sensor(30,false,1700),remote(Motion::Stop,1700,3),p,1700);out=updateController(c,sensor(30,false,2100),remote(Motion::Stop,2100,3),p,2100);assert(c.phase==Phase::AwaitNewCommand&&out.motion==Motion::Stop);
RemoteState rs;acceptRemote(rs,Motion::Forward,20,false);assert(rs.sequence==1);acceptRemote(rs,Motion::Forward,30,true);assert(rs.sequence==1&&rs.receivedAt==30);
assert(buzzerGate(true,0));assert(!buzzerGate(true,200));assert(!buzzerGate(false,0));
puts("PASS: boundaries, wraparound, sensor validity, motor mapping, command loss, escape, latch, STOP priority, timeout, recovery and repeat handling");}
