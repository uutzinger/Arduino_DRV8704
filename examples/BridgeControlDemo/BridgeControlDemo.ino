#include <drv8704.h>
#include <logger.h>

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
  currentLogLevel = LOG_LEVEL_INFO;

  DRV8704& driver = motorDriver();
  if (!driver.begin()) {
    Serial.println("DRV8704 begin failed");
    return;
  }

  DRV8704Config config;
  config.enabled = true;
  config.torque = 160;
  config.decayMode = DecayMode::Mixed;
  driver.applyConfig(config);

  driver.setBridgeMode(BridgeId::A, BridgeMode::Coast);
}

void loop() {
  DRV8704& driver = motorDriver();

  driver.setBridgeMode(BridgeId::A, BridgeMode::Forward);
  delay(1500);

  driver.setBridgeMode(BridgeId::A, BridgeMode::Brake);
  delay(500);

  driver.setBridgeMode(BridgeId::A, BridgeMode::Reverse);
  delay(1500);

  driver.setBridgeMode(BridgeId::A, BridgeMode::Coast);
  delay(1000);
}
