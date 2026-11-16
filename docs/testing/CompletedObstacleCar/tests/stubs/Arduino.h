#pragma once
#include <stdint.h>
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
extern uint32_t fakeNow;
extern int pins[32],pwm[32],modes[32];
extern int reads,writes,analogCalls,modeCalls,pulseCalls,delayCalls;
extern int pulsePin,pulseLevel;
extern unsigned long echoUs,pulseTimeout;
inline unsigned long millis(){return fakeNow;}
inline void pinMode(int p,int v){modes[p]=v;++modeCalls;}
inline int digitalRead(int p){++reads;return pins[p];}
inline void digitalWrite(int p,int v){pins[p]=v;++writes;}
inline void analogWrite(int p,int v){pwm[p]=v;++analogCalls;}
inline void delayMicroseconds(unsigned int){++delayCalls;}
inline unsigned long pulseIn(int p,int l,unsigned long t){++pulseCalls;pulsePin=p;pulseLevel=l;pulseTimeout=t;return echoUs;}
