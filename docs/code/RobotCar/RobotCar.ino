// Reference integration, NOT verified against the physical OSOYOO kit.
// Open this directory as an Arduino sketch. Install Arduino-IRremote 4.x.
// Unconfigured defaults never initialize motor outputs. No arbitrary pin map is supplied.
#include "HardwareAdapter.h"
#include "RemoteAdapter.h"
robot::Parameters parameters;
robot::Controller controller;
robot::RemoteState remoteState;
robot::SensorFrame sensors;
uint32_t lastSample=0;
uint32_t lastReport=0;
bool ready=false;
void setup() {
 Serial.begin(115200);
 ready=robot::validParameters(parameters)&&remote::codesReady()&&hardware::initializeHardware();
 robot::resetController(controller,millis());
 if(!ready){Serial.println(F("CONFIGURATION NOT CONFIRMED: motors disabled"));return;}
 hardware::stopMotors();
 IrReceiver.begin(board::IR_RECEIVE,DISABLE_LED_FEEDBACK);
}
void loop() {
 if(!ready)return;
 uint32_t now=millis();
 remote::pollRemote(remoteState,now,parameters);
 // Stop command can cut PWM before a bounded ultrasonic measurement begins.
 if(remoteState.command==robot::Motion::Stop)hardware::stopMotors();
 if(robot::elapsed(now,lastSample,board::SAMPLE_PERIOD_MS)){
  lastSample=now;sensors=hardware::sampleSensors(now);
 }
 // Refresh now after pulseIn so freshness includes acquisition time.
 now=millis();
 robot::Output out=robot::updateController(controller,sensors,remoteState,parameters,now);
 hardware::applyOutput(out,now);
 if(robot::elapsed(now,lastReport,250)){
  lastReport=now;
  Serial.print(F("cm="));Serial.print(sensors.frontCm);
  Serial.print(F(",rear="));Serial.print(sensors.rearBlocked);
  Serial.print(F(",motion="));Serial.print(static_cast<int>(out.motion));
  Serial.print(F(",reason="));Serial.println(static_cast<int>(out.reason));
 }
}
