#pragma once
#include <stdint.h>
// This is NOT an OSOYOO pin map. Replace only after checking the actual board and wiring.
// Adapter assumes a dual H-bridge with IN1/IN2/PWM for each motor. Other drivers need a new adapter.
namespace board {
constexpr bool WIRING_CONFIRMED=false;
constexpr bool REMOTE_CODES_CONFIRMED=false;
constexpr bool STOP_BOUNDARY_CONFIRMED=false; // 5cm <= versus < remains an open requirement.
constexpr bool ACTIVE_BUZZER_CONFIRMED=false; // digital ON/OFF type only, not passive tone().
constexpr int8_t LEFT_IN1=-1,LEFT_IN2=-1,LEFT_PWM=-1;
constexpr int8_t RIGHT_IN1=-1,RIGHT_IN2=-1,RIGHT_PWM=-1;
constexpr int8_t TRIG=-1,ECHO=-1,REAR=-1,BUZZER=-1,IR_RECEIVE=-1;
constexpr bool LEFT_INVERTED=false,RIGHT_INVERTED=false;
constexpr bool REAR_ACTIVE_LOW=true; // Example polarity. Verify with the sensor diagnostic.
constexpr bool BUZZER_ACTIVE_HIGH=true;
constexpr uint16_t REMOTE_ADDRESS=0;
constexpr uint16_t KEY_FORWARD=0,KEY_BACKWARD=0,KEY_LEFT=0,KEY_RIGHT=0,KEY_STOP=0;
constexpr uint32_t SAMPLE_PERIOD_MS=60;
constexpr uint32_t ECHO_TIMEOUT_US=15000;
}
