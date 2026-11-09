#pragma once
#include <Arduino.h>
#include "BoardConfig.h"
#include "RobotLogic.h"
namespace hardware {
// @function validPin
inline bool validPin(int8_t pin) { return pin>=2&&pin<=19; } // D0/D1 reserved for Serial on UNO.
// @endfunction
// @function pwmPin
inline bool pwmPin(int8_t pin) { return pin==5||pin==6||pin==9||pin==10; } // Avoid Timer2 PWM when using IRremote.
// @endfunction
// @function pinsReady
inline bool pinsReady() {
 const int8_t pins[]={board::LEFT_IN1,board::LEFT_IN2,board::LEFT_PWM,board::RIGHT_IN1,board::RIGHT_IN2,board::RIGHT_PWM,board::TRIG,board::ECHO,board::REAR,board::BUZZER,board::IR_RECEIVE};
 for(uint8_t i=0;i<sizeof(pins);++i){if(!validPin(pins[i]))return false;for(uint8_t j=0;j<i;++j)if(pins[i]==pins[j])return false;}
 return board::WIRING_CONFIRMED&&board::STOP_BOUNDARY_CONFIRMED&&board::ACTIVE_BUZZER_CONFIRMED&&pwmPin(board::LEFT_PWM)&&pwmPin(board::RIGHT_PWM);
}
// @endfunction
// @function initializeHardware
inline bool initializeHardware() {
 if(!pinsReady())return false;
 const int8_t outs[]={board::LEFT_IN1,board::LEFT_IN2,board::LEFT_PWM,board::RIGHT_IN1,board::RIGHT_IN2,board::RIGHT_PWM,board::TRIG};
 for(uint8_t i=0;i<sizeof(outs);++i){digitalWrite(outs[i],LOW);pinMode(outs[i],OUTPUT);}
 digitalWrite(board::BUZZER,board::BUZZER_ACTIVE_HIGH?LOW:HIGH);pinMode(board::BUZZER,OUTPUT);
 pinMode(board::ECHO,INPUT);pinMode(board::REAR,INPUT_PULLUP);pinMode(board::IR_RECEIVE,INPUT);
 analogWrite(board::LEFT_PWM,0);analogWrite(board::RIGHT_PWM,0);return true;
}
// @endfunction
// @function writeWheel
inline void writeWheel(int8_t in1,int8_t in2,int8_t pwm,int16_t speed,bool inverted) {
 if(!pinsReady())return;
 int16_t v=inverted?-speed:speed;v=v>255?255:v<-255?-255:v;
 // Disable PWM before changing direction. Stop is driver-dependent coasting, not guaranteed braking.
 analogWrite(pwm,0);digitalWrite(in1,v>0?HIGH:LOW);digitalWrite(in2,v<0?HIGH:LOW);analogWrite(pwm,robot::clampPwm(v<0?-v:v));
}
// @endfunction
// @function stopMotors
inline void stopMotors() { if(!pinsReady())return;writeWheel(board::LEFT_IN1,board::LEFT_IN2,board::LEFT_PWM,0,false);writeWheel(board::RIGHT_IN1,board::RIGHT_IN2,board::RIGHT_PWM,0,false); }
// @endfunction
// @function readUltrasonic
inline float readUltrasonic() {
 if(!pinsReady())return NAN;
 digitalWrite(board::TRIG,LOW);delayMicroseconds(2);digitalWrite(board::TRIG,HIGH);delayMicroseconds(10);digitalWrite(board::TRIG,LOW);
 // Bounded blocking call: at most the timeout, not a fully asynchronous sensor driver.
 const unsigned long us=pulseIn(board::ECHO,HIGH,board::ECHO_TIMEOUT_US);
 return robot::pulseToCm(us);
}
// @endfunction
// @function sampleSensors
inline robot::SensorFrame sampleSensors(uint32_t now) {
 robot::SensorFrame s;s.sampledAt=now;
 if(!pinsReady())return s;
 s.frontCm=readUltrasonic();s.frontValid=robot::validDistance(s.frontCm);
 s.rearBlocked=robot::rearFromLevel(digitalRead(board::REAR)==HIGH,board::REAR_ACTIVE_LOW);s.rearValid=true;
 // A digital read does NOT prove a cable is intact or detect a stuck IR sensor.
 return s;
}
// @endfunction
// @function applyOutput
inline void applyOutput(const robot::Output& o,uint32_t now) {
 if(!pinsReady())return;
 const robot::WheelPair w=robot::wheelsFor(o.motion,o.pwm);
 writeWheel(board::LEFT_IN1,board::LEFT_IN2,board::LEFT_PWM,w.left,board::LEFT_INVERTED);
 writeWheel(board::RIGHT_IN1,board::RIGHT_IN2,board::RIGHT_PWM,w.right,board::RIGHT_INVERTED);
 bool on=robot::buzzerGate(o.buzzer,now);
 digitalWrite(board::BUZZER,(on==board::BUZZER_ACTIVE_HIGH)?HIGH:LOW);
}
// @endfunction
}
