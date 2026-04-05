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

void printPwmCapability(Stream& out, const DRV8704PwmCapability& capability) {
  out.print("Requested frequency (Hz): ");
  out.println(capability.requestedFrequencyHz);
  out.print("Achieved frequency (Hz): ");
  out.println(capability.achievedFrequencyHz);
  out.print("Resolution bits: ");
  out.println(capability.resolutionBits);
  out.print("Duty steps: ");
  out.println(capability.dutySteps);
  out.print("Smallest increment (%): ");
  out.println(capability.smallestDutyIncrementPercent, 6);
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

  DRV8704PwmConfig pwmConfig;
  pwmConfig.frequencyHz = 20000UL;
  pwmConfig.preferredResolutionBits = 10U;

  if (!driver.setCurrentLimit(4.0f)) {
    Serial.println("Current limit setup failed");
    return;
  }

  if (!driver.beginPwmMode(pwmConfig)) {
    Serial.println("PWM mode unavailable on this target");
    return;
  }

  driver.setDirection(BridgeId::A, Direction::Forward);
  printPwmCapability(Serial, driver.pwmCapability());
}

void loop() {
  DRV8704& driver = motorDriver();

  driver.setDirection(BridgeId::A, Direction::Forward);
  driver.setSpeed(BridgeId::A, 25.0f);
  delay(1500);

  driver.setSpeed(BridgeId::A, 60.0f);
  delay(1500);

  driver.brake(BridgeId::A);
  delay(500);

  driver.setDirection(BridgeId::A, Direction::Reverse);
  driver.setSpeed(BridgeId::A, 100.0f);
  delay(1000);

  driver.setDirection(BridgeId::A, Direction::Forward);
  driver.setCurrent(BridgeId::A, 2.5f);
  delay(1000);

  driver.setSpeed(BridgeId::A, 0.0f);
  delay(1000);
}
