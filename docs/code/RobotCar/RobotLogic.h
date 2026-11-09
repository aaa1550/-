#pragma once
#include <stdint.h>
#include <math.h>
// Portable logic. All timings and speeds below are example parameters, not kit specifications.
namespace robot {
enum class Motion : uint8_t { Stop, Forward, Backward, Left, Right };
enum class Phase : uint8_t { Idle, StopBeforeEscape, EscapeBackward, EscapeForward, AwaitNewCommand, LatchedStop };
enum class Reason : uint8_t { None, ManualStop, RemoteLost, FrontEmergency, SensorFault, BothBlocked, AvoidingFront, AvoidingRear, EscapeTimeout, AwaitFreshCommand };
struct Parameters { float warningCm=15.0f; float emergencyCm=5.0f; bool emergencyInclusive=true; uint8_t normalPwm=120; uint8_t escapePwm=70; uint32_t sensorMaxAgeMs=200; uint32_t remoteTimeoutMs=350; uint32_t stopBeforeReverseMs=100; uint32_t maxEscapeMs=1500; uint32_t clearHoldMs=400; };
struct SensorFrame { float frontCm=0; bool frontValid=false; bool rearBlocked=true; bool rearValid=false; uint32_t sampledAt=0; };
struct RemoteState { Motion command=Motion::Stop; bool valid=false; uint32_t receivedAt=0; uint32_t sequence=0; };
struct Controller { Phase phase=Phase::Idle; uint32_t phaseStarted=0; uint32_t clearStarted=0; bool clearTiming=false; uint32_t blockedSequence=0; Reason latchedReason=Reason::None; };
struct Output { Motion motion; uint8_t pwm; bool buzzer; Reason reason; Output(Motion m=Motion::Stop,uint8_t v=0,bool b=false,Reason r=Reason::None):motion(m),pwm(v),buzzer(b),reason(r){} };
struct WheelPair { int16_t left; int16_t right; WheelPair(int16_t l=0,int16_t r=0):left(l),right(r){} };
// @function elapsed
inline bool elapsed(uint32_t now,uint32_t since,uint32_t interval) { return uint32_t(now-since)>=interval; }
// @endfunction
// @function clampPwm
inline uint8_t clampPwm(int value) { return value<0?0:value>255?255:static_cast<uint8_t>(value); }
// @endfunction
// @function validParameters
inline bool validParameters(const Parameters& p) {
 return isfinite(p.warningCm)&&isfinite(p.emergencyCm)&&p.emergencyCm>0&&p.warningCm>p.emergencyCm&&p.normalPwm>0&&p.escapePwm>0&&p.escapePwm<=p.normalPwm&&p.sensorMaxAgeMs>0&&p.remoteTimeoutMs>0&&p.stopBeforeReverseMs>0&&p.maxEscapeMs>p.stopBeforeReverseMs&&p.clearHoldMs>0&&p.maxEscapeMs<0x80000000UL&&p.sensorMaxAgeMs<0x80000000UL&&p.remoteTimeoutMs<0x80000000UL&&p.stopBeforeReverseMs<0x80000000UL&&p.clearHoldMs<0x80000000UL;
}
// @endfunction
// @function pulseToCm
inline float pulseToCm(uint32_t pulseUs) { return pulseUs==0?NAN:static_cast<float>(pulseUs)*0.0343f/2.0f; }
// @endfunction
// @function validDistance
inline bool validDistance(float cm) { return isfinite(cm)&&cm>=2.0f&&cm<=200.0f; }
// @endfunction
// @function frontEmergency
inline bool frontEmergency(float cm,const Parameters& p) { return isfinite(cm)&&(p.emergencyInclusive?cm<=p.emergencyCm:cm<p.emergencyCm); }
// @endfunction
// @function frontWarning
inline bool frontWarning(float cm,const Parameters& p) { return isfinite(cm)&&cm<=p.warningCm; }
// @endfunction
// @function rearFromLevel
inline bool rearFromLevel(bool levelHigh,bool activeLow) { return activeLow?!levelHigh:levelHigh; }
// @endfunction
// @function sensorsFresh
inline bool sensorsFresh(const SensorFrame& s,uint32_t now,const Parameters& p) { return s.frontValid&&validDistance(s.frontCm)&&s.rearValid&&!elapsed(now,s.sampledAt,p.sensorMaxAgeMs); }
// @endfunction
// @function remoteFresh
inline bool remoteFresh(const RemoteState& r,uint32_t now,const Parameters& p) { return r.valid&&!elapsed(now,r.receivedAt,p.remoteTimeoutMs); }
// @endfunction
// @function motionAllowed
inline bool motionAllowed(Motion m,const SensorFrame& s,const Parameters& p) {
 if(m==Motion::Stop)return true;
 if(!s.frontValid||!validDistance(s.frontCm)||!s.rearValid)return false;
 if(m==Motion::Forward)return !frontWarning(s.frontCm,p);
 if(m==Motion::Backward)return !s.rearBlocked;
 // Side clearance is not measured; permit turns only when both longitudinal directions are clear.
 return !frontWarning(s.frontCm,p)&&!s.rearBlocked;
}
// @endfunction
// @function wheelsFor
inline WheelPair wheelsFor(Motion m,uint8_t pwm) {
 int16_t v=pwm;const int16_t n=static_cast<int16_t>(-v);
 switch(m){case Motion::Forward:return {v,v};case Motion::Backward:return {n,n};case Motion::Left:return {n,v};case Motion::Right:return {v,n};default:return {0,0};}
}
// @endfunction
// @function median3
inline float median3(float a,float b,float c) { if(!isfinite(a)||!isfinite(b)||!isfinite(c))return NAN; if(a>b){float t=a;a=b;b=t;}if(b>c){float t=b;b=c;c=t;}return a>b?a:b; }
// @endfunction
// @function legacyRearState
inline uint8_t legacyRearState(bool blocked) { return blocked?2:3; }
// @endfunction
// @function makeStop
inline Output makeStop(Reason reason,bool warning=false) { return {Motion::Stop,0,warning,reason}; }
// @endfunction
// @function resetController
inline void resetController(Controller& c,uint32_t now) { c=Controller{};c.phaseStarted=now; }
// @endfunction
// @function enterPhase
inline void enterPhase(Controller& c,Phase p,uint32_t now,uint32_t sequence) { c.phase=p;c.phaseStarted=now;c.clearTiming=false;c.blockedSequence=sequence; }
// @endfunction
// @function clearStable
inline bool clearStable(Controller& c,bool clear,uint32_t now,uint32_t holdMs) {
 if(!clear){c.clearTiming=false;return false;}
 if(!c.clearTiming){c.clearTiming=true;c.clearStarted=now;}
 return elapsed(now,c.clearStarted,holdMs);
}
// @endfunction
// @function latchStop
inline Output latchStop(Controller& c,Reason reason,uint32_t now,uint32_t seq) { enterPhase(c,Phase::LatchedStop,now,seq);c.latchedReason=reason;return makeStop(reason,true); }
// @endfunction
// @function acceptRemote
inline void acceptRemote(RemoteState& r,Motion command,uint32_t now,bool repeat) { if(!repeat){r.command=command;r.sequence++;}r.receivedAt=now;r.valid=true; }
// @endfunction
// @function buzzerGate
inline bool buzzerGate(bool warning,uint32_t now,uint32_t periodMs=400) { return warning&&periodMs>1&&(now%periodMs)<periodMs/2; }
// @endfunction
// @function updateController
inline Output updateController(Controller& c,const SensorFrame& s,const RemoteState& r,const Parameters& p,uint32_t now) {
 if(!validParameters(p))return latchStop(c,Reason::SensorFault,now,r.sequence);
 if(!sensorsFresh(s,now,p))return latchStop(c,Reason::SensorFault,now,r.sequence);
 const bool front=frontWarning(s.frontCm,p),both=front&&s.rearBlocked,allClear=!front&&!s.rearBlocked;
 const bool dangerous=frontEmergency(s.frontCm,p);
 if(c.phase==Phase::LatchedStop){
  // Explicit STOP acknowledgement with fresh sensors and stable clearance is required.
  if(dangerous||both){c.clearTiming=false;return makeStop(c.latchedReason,true);}
  const bool ack=remoteFresh(r,now,p)&&r.command==Motion::Stop&&r.sequence!=c.blockedSequence;
  if(clearStable(c,allClear&&ack,now,p.clearHoldMs)){enterPhase(c,Phase::AwaitNewCommand,now,r.sequence);return makeStop(Reason::AwaitFreshCommand);}
  return makeStop(c.latchedReason,true);
 }
 if(dangerous)return latchStop(c,Reason::FrontEmergency,now,r.sequence);
 if(both)return latchStop(c,Reason::BothBlocked,now,r.sequence);
 if(!remoteFresh(r,now,p)){enterPhase(c,Phase::AwaitNewCommand,now,r.sequence);return makeStop(Reason::RemoteLost);}
 if(r.command==Motion::Stop){enterPhase(c,Phase::Idle,now,r.sequence);return makeStop(Reason::ManualStop);}
 if(c.phase==Phase::AwaitNewCommand){if(r.sequence==c.blockedSequence)return makeStop(Reason::AwaitFreshCommand);enterPhase(c,Phase::Idle,now,r.sequence);}
 if(c.phase==Phase::Idle){
  const bool avoidFront=front&&(r.command==Motion::Forward||r.command==Motion::Left||r.command==Motion::Right);
  const bool avoidRear=s.rearBlocked&&(r.command==Motion::Backward||r.command==Motion::Left||r.command==Motion::Right);
  if(avoidFront||avoidRear){enterPhase(c,Phase::StopBeforeEscape,now,r.sequence);c.latchedReason=avoidFront?Reason::AvoidingFront:Reason::AvoidingRear;return makeStop(c.latchedReason,true);}
  return motionAllowed(r.command,s,p)?Output{r.command,p.normalPwm,false,Reason::None}:makeStop(Reason::ManualStop,true);
 }
 if(c.phase==Phase::StopBeforeEscape){
  if(!elapsed(now,c.phaseStarted,p.stopBeforeReverseMs))return makeStop(c.latchedReason,true);
  const bool back=c.latchedReason==Reason::AvoidingFront;
  const Motion target=back?Motion::Backward:Motion::Forward;
  if(!motionAllowed(target,s,p))return latchStop(c,Reason::BothBlocked,now,r.sequence);
  enterPhase(c,back?Phase::EscapeBackward:Phase::EscapeForward,now,r.sequence);
 }
 if(c.phase==Phase::EscapeBackward||c.phase==Phase::EscapeForward){
  if(elapsed(now,c.phaseStarted,p.maxEscapeMs))return latchStop(c,Reason::EscapeTimeout,now,r.sequence);
  if(clearStable(c,allClear,now,p.clearHoldMs)){enterPhase(c,Phase::AwaitNewCommand,now,r.sequence);return makeStop(Reason::AwaitFreshCommand);}
  const Motion target=c.phase==Phase::EscapeBackward?Motion::Backward:Motion::Forward;
  if(!motionAllowed(target,s,p))return latchStop(c,Reason::BothBlocked,now,r.sequence);
  // Once allClear becomes true, hold STOP during clearance confirmation rather than continue driving.
  return allClear?makeStop(c.latchedReason,true):Output{target,p.escapePwm,true,c.latchedReason};
 }
 return makeStop(Reason::ManualStop);
}
// @endfunction
}
