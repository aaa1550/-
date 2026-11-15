#pragma once
#include <stdint.h>
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
extern uint32_t fakeNow, fakeMicros;
extern uint32_t pinHighAt[32], pinLowAt[32];
extern int pinWrites[32];
extern int pins[32],pwm[32];
extern unsigned long echoUs;
inline unsigned long millis(){return fakeNow;}
inline unsigned long micros(){return fakeMicros++;}
inline void pinMode(int,int){}
inline int digitalRead(int p){return pins[p];}
inline void digitalWrite(int p,int v){pins[p]=v;++pinWrites[p];if(v==HIGH)pinHighAt[p]=fakeMicros;else pinLowAt[p]=fakeMicros;}
inline void analogWrite(int p,int v){pwm[p]=v;}
inline void delayMicroseconds(unsigned int){}
inline void delay(unsigned long ms){fakeNow+=ms;}
inline unsigned long pulseIn(int,int,unsigned long){return echoUs;}
