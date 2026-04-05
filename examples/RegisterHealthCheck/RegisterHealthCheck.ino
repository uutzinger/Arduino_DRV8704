#include <drv8704.h>
#include <logger.h>

namespace {

DRV8704Pins makePins() {
  DRV8704Pins pins;
  pins.csPin = 5;
  pins.sleepPin = 11;
  pins.resetPin = A0;
  pins.faultPin = A1;
  return pins;
}

DRV8704& motorDriver() {
  static DRV8704Pins pins = makePins();
  static DRV8704 driver(pins);
  return driver;
}

void printRegister(Stream& out, const char* label, uint16_t value) {
  out.print(label);
  out.print(": 0x");
  if (value < 0x1000) {
    out.print('0');
  }
  out.println(value, HEX);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  currentLogLevel = LOG_LEVEL_INFO;
  unsigned long start = millis();
  while (!Serial && millis() - start < 10000) {
      delay(10);
  }  

  // Deselect other SPI devices that may be present on the bus
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);

  DRV8704& driver = motorDriver();
  if (!driver.begin()) {
    Serial.println("DRV8704 begin failed");
    return;
  }

  DRV8704HealthCheck health = driver.healthCheck();
  Serial.print("Initialized: ");
  Serial.println(health.initialized ? "yes" : "no");
  Serial.print("SPI ok: ");
  Serial.println(health.spiOk ? "yes" : "no");
  Serial.print("Fault pin wired: ");
  Serial.println(driver.hasFaultPin() ? "yes" : "no");
  Serial.print("Fault pin active: ");
  Serial.println(driver.isFaultPinActive() ? "yes" : "no");
  Serial.print("Fault present: ");
  Serial.println(health.faultPresent ? "yes" : "no");

  if (!driver.clearFaults()) {
    Serial.println("Clear faults command failed");
  } else {
    Serial.println("Clear faults command sent");
  }

  DRV8704Status status = driver.readStatus();
  Serial.print("Fault present after clear: ");
  Serial.println(driver.hasFault() ? "yes" : "no");

  printRegister(Serial, "CTRL", driver.readRegister(RegisterAddress::Ctrl));
  printRegister(Serial, "TORQUE", driver.readRegister(RegisterAddress::Torque));
  printRegister(Serial, "OFF", driver.readRegister(RegisterAddress::Off));
  printRegister(Serial, "BLANK", driver.readRegister(RegisterAddress::Blank));
  printRegister(Serial, "DECAY", driver.readRegister(RegisterAddress::Decay));
  printRegister(Serial, "DRIVE", driver.readRegister(RegisterAddress::Drive));
  printRegister(Serial, "STATUS", driver.readRegister(RegisterAddress::Status));

  Serial.print("Overtemperature: ");
  Serial.println(status.overTemperature ? "yes" : "no");
  Serial.print("A overcurrent: ");
  Serial.println(status.overCurrentA ? "yes" : "no");
  Serial.print("B overcurrent: ");
  Serial.println(status.overCurrentB ? "yes" : "no");
  Serial.print("A predriver fault: ");
  Serial.println(status.predriverFaultA ? "yes" : "no");
  Serial.print("B predriver fault: ");
  Serial.println(status.predriverFaultB ? "yes" : "no");
  Serial.print("Undervoltage: ");
  Serial.println(status.undervoltage ? "yes" : "no");
}

void loop() {
}
