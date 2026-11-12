#ifndef FLOWCHART_LOGIC_H
#define FLOWCHART_LOGIC_H
#include <stdint.h>
namespace flowcar {
// RMは原図の値。右=3、左=4。
enum Remote : uint8_t { STOP=0, FORWARD=1, BACKWARD=2, RIGHT=3, LEFT=4 };
enum Front : uint8_t { CLEAR, CAUTION, EMERGENCY, INVALID };
enum Phase : uint8_t { MANUAL, BRAKING, ESCAPING, WAIT_NEW };
struct Sensors { Front front; bool rear; };
struct Output { int8_t right; int8_t left; bool buzzer; uint8_t m; };
struct Controller {
  Phase phase=WAIT_NEW;
  uint32_t started=0;
  uint32_t lastSequence=0;
  bool escapeBackward=false;
};
inline Front classifyFront(unsigned long echoUs) {
  if(echoUs==0) return INVALID;
  // 元コードと同じ換算係数0.01657。丸める前の距離で境界判定。
  const float cm=echoUs*0.01657f;
  if(cm<=5.0f) return EMERGENCY;
  if(cm<=15.0f) return CAUTION;
  return CLEAR;
}
inline Output stop(bool alarm=false,uint8_t m=0) { return {0,0,alarm,m}; }
inline Output manual(Remote rm) {
  switch(rm) {
    case FORWARD: return {50,50,false,3};
    case BACKWARD: return {-30,-30,false,4};
    case LEFT: return {30,-30,false,5};
    case RIGHT: return {-30,30,false,6};
    default: return stop();
  }
}
// 非ブロッキングの停止→反対方向10%→停止。回避完了後は新しい操作待ち。
inline Output update(Controller& c,Sensors s,Remote rm,bool linkAlive,
                     uint32_t sequence,uint32_t now) {
  const bool fresh=sequence!=c.lastSequence;
  c.lastSequence=sequence;
  const bool frontBlocked=s.front==CAUTION||s.front==EMERGENCY;
  const bool alarm=s.front==INVALID||frontBlocked||s.rear;
  if(!linkAlive||rm==STOP) { c.phase=WAIT_NEW;return stop(alarm); }
  if(s.front==INVALID) { c.phase=WAIT_NEW;return stop(true,8); }
  if(s.front==EMERGENCY) { c.phase=WAIT_NEW;return stop(true,1); }
  if(frontBlocked&&s.rear) { c.phase=WAIT_NEW;return stop(true,7); }
  if(c.phase==WAIT_NEW) {
    if(!fresh) return stop(alarm);
    c.phase=MANUAL;
  }
  if(c.phase==BRAKING||c.phase==ESCAPING) {
    // 回避方向も毎ループ監視。前後同時・5cm以下は上で停止する。
    const bool obstacleRemains=c.escapeBackward?frontBlocked:s.rear;
    if(!obstacleRemains) { c.phase=WAIT_NEW;return stop(alarm); }
    if(uint32_t(now-c.started)>=1500U) { c.phase=WAIT_NEW;return stop(true); }
    if(c.phase==BRAKING) {
      if(uint32_t(now-c.started)<100U) return stop(true,c.escapeBackward?1:2);
      c.phase=ESCAPING;
    }
    return c.escapeBackward?Output{-10,-10,true,1}:Output{10,10,true,2};
  }
  // 原図の「前方→後方→リモコン」の優先順。起動直後やSTOP中は自走しない。
  if(frontBlocked||s.rear) {
    c.escapeBackward=frontBlocked;
    c.started=now;c.phase=BRAKING;
    return stop(true,frontBlocked?1:2);
  }
  return manual(rm);
}
}
#endif
