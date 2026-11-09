#pragma once
#include <stdint.h>
#include <math.h>
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
#define LED_BUILTIN 13
#define HEX 16
#define F(x) x
inline void pinMode(int,int){} inline void digitalWrite(int,int){} inline int digitalRead(int){return 1;}
inline void analogWrite(int,int){} inline void delayMicroseconds(unsigned int){} inline unsigned long pulseIn(int,int,unsigned long){return 1000;}
inline uint32_t millis(){return 0;}
struct MockSerial {void begin(unsigned long){}int available(){return 0;}char read(){return 0;}template<class T>void print(T){}template<class T>void print(T,int){}template<class T>void println(T){}void println(){}};
static MockSerial Serial;
