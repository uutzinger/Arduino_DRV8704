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

void printCurrentLimit(Stream& out, const DRV8704CurrentLimitResult& result) {
  out.print("Requested current limit (A): ");
  out.println(result.requestedCurrentLimitAmps, 4);
  out.print("Applied A limit (A): ");
  out.println(result.appliedCurrentLimitAmpsA, 4);
  out.print("Applied B limit (A): ");
  out.println(result.appliedCurrentLimitAmpsB, 4);
  out.print("Reference shunt (ohms): ");
  out.println(result.referenceShuntOhms, 6);
  out.print("Torque DAC: ");
  out.println(result.torqueDac);
  out.print("Max current (A): ");
  out.println(result.achievableMaxCurrentAmps, 4);
  out.print("Resolution (A/count): ");
  out.println(result.ampsPerTorqueCount, 6);
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

  driver.setShuntResistance(0.0005f);
  driver.setCurrentModePreset(CurrentModePreset::MotorMediumInductance);
  driver.setDirection(BridgeId::A, Direction::Forward);

  if (!driver.setCurrent(BridgeId::A, 4.0f)) {
    Serial.println("Current mode request failed");
    return;
  }

  printCurrentLimit(Serial, driver.currentLimitResult());
}

void loop() {
  DRV8704& driver = motorDriver();

  driver.setDirection(BridgeId::A, Direction::Forward);
  driver.setCurrent(BridgeId::A, 2.0f);
  delay(1500);

  driver.setCurrent(BridgeId::A, 4.0f);
  delay(1500);

  driver.setDirection(BridgeId::A, Direction::Reverse);
  driver.setCurrent(BridgeId::A, 3.0f);
  delay(1500);

  driver.coast(BridgeId::A);
  delay(1000);
}
