#include <drv8704.h>

namespace {

DRV8704Pins makePins() {
  DRV8704Pins pins;
  pins.csPin = 5;
  pins.sleepPin = 11;
  pins.resetPin = A0;
  pins.faultPin = A1;
  pins.ain1Pin = A2;
  pins.ain2Pin = A3;
  pins.bin1Pin = A4;
  pins.bin2Pin = A5;
  return pins;
}

DRV8704& motorDriver() {
  static DRV8704Pins pins = makePins();
  static DRV8704 driver(pins);
  return driver;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);

  DRV8704& driver = motorDriver();
  if (!driver.begin()) {
    Serial.println("DRV8704 begin failed");
    return;
  }

  DRV8704Config config;
  config.enabled = true;
  config.senseGain = SenseGain::Gain20VperV;
  config.deadTime = DeadTime::Ns460;
  config.torque = 180;
  config.offTime = 0x30;
  config.blankTime = 0x80;
  config.decayMode = DecayMode::Mixed;
  config.decayTime = 0x10;

  if (!driver.applyConfig(config)) {
    Serial.println("DRV8704 config failed");
    return;
  }

  Serial.println("DRV8704 basic bring-up complete");
}

void loop() {
}
