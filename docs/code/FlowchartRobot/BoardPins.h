#ifndef FLOWCHART_BOARD_PINS_H
#define FLOWCHART_BOARD_PINS_H
#include <Arduino.h>
// ２.jpg / 4.jpg / 配線.jpg / 6.jpg / 3.jpg / 7.png を照合。
// false: 前版と同じ。true: 配線.jpgのEcho D2を採用し、右後方信号はA5へ移す。
// 切り替える場合はEchoと右後方センサーの実配線も変更する。
constexpr bool USE_PHOTO_ECHO_WIRING=false;
constexpr uint8_t IR_RECEIVE_PIN=4;
constexpr uint8_t speedPinR=5;       // 元code.txtの9から変更（ENAの線を移す）
constexpr uint8_t RightDirectPin1=12;
constexpr uint8_t RightDirectPin2=11;
constexpr uint8_t speedPinL=6;
constexpr uint8_t LeftDirectPin1=7;
constexpr uint8_t LeftDirectPin2=8;
constexpr uint8_t BUZZ_PIN=13;       // LOWで鳴動するアクティブブザー
constexpr uint8_t LFSensor_bl=0;     // code.txtを継承。D0のためSerialは使わない
constexpr uint8_t LFSensor_br=USE_PHOTO_ECHO_WIRING?A5:2;
constexpr uint8_t Echo_PIN=USE_PHOTO_ECHO_WIRING?2:3; // 画像はD2、code.txtはD3
constexpr uint8_t Trig_PIN=10;
constexpr uint8_t SERVO_PIN=9;       // 元code.txtの5から変更（サーボの信号線を移す）

namespace wiringcheck {
constexpr bool contains(int) {return false;}
template<typename... Rest> constexpr bool contains(int pin,int next,Rest... rest) {
  return pin==next||contains(pin,rest...);
}
constexpr bool unique() {return true;}
template<typename... Rest> constexpr bool unique(int pin,Rest... rest) {
  return !contains(pin,rest...)&&unique(rest...);
}
}
static_assert(wiringcheck::unique(IR_RECEIVE_PIN,speedPinR,RightDirectPin1,
 RightDirectPin2,speedPinL,LeftDirectPin1,LeftDirectPin2,BUZZ_PIN,
 LFSensor_bl,LFSensor_br,Echo_PIN,Trig_PIN,SERVO_PIN),"Pin collision: check BoardPins.h and wiring");
// Servo=Timer1、IRremote=Timer2のためモーターPWMはTimer0のD5/D6を使用。
static_assert((speedPinR==5||speedPinR==6)&&(speedPinL==5||speedPinL==6),
 "UNO motor PWM must use D5/D6 with Servo and Timer2 IRremote");
#endif
