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

void printPreset(Stream& out, const DRV8704CurrentPresetConfig& preset) {
  out.print("TOFF: 0x");
  out.println(preset.offTime, HEX);
  out.print("TBLANK: 0x");
  out.println(preset.blankTime, HEX);
  out.print("TDECAY: 0x");
  out.println(preset.decayTime, HEX);
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
  driver.setDirection(BridgeId::A, Direction::Forward);

  driver.setCurrentModePreset(CurrentModePreset::Heater);
  Serial.println("Heater preset:");
  printPreset(Serial, driver.currentModePresetConfig());
  driver.setCurrent(BridgeId::A, 2.0f);
  delay(2000);

  driver.setCurrentModePreset(CurrentModePreset::ThermoelectricCooler);
  Serial.println("TEC preset:");
  printPreset(Serial, driver.currentModePresetConfig());
  driver.setCurrent(BridgeId::A, 2.0f);
  delay(2000);

  driver.coast(BridgeId::A);
}

void loop() {
}
