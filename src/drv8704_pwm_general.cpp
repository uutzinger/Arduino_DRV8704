/**
 * @file drv8704_pwm_general.cpp
 * @brief Generic Arduino analogWrite backend for DRV8704 PWM input generation.
 */

#include "drv8704_pwm.h"

#if !defined(ARDUINO_ARCH_ESP32) && \
    !defined(CORE_TEENSY) && \
    !defined(ARDUINO_ARCH_STM32) && \
    !defined(ARDUINO_ARCH_STM32F1) && \
    !defined(ARDUINO_ARCH_STM32L4)

namespace {

static const uint8_t kGeneralResolutionBits = 8U;
static const uint32_t kGeneralDutySteps = 256UL;

class GeneralAnalogWriteBackend : public DRV8704PwmBackend {
public:
  bool begin(const DRV8704Pins& pins,
             const DRV8704PwmConfig& config,
             DRV8704PwmCapability& capability) override {
    initializePins(pins);
    return configure(pins, config, capability);
  }

  void end(const DRV8704Pins& pins) override {
    releasePin(pins.ain1Pin);
    releasePin(pins.ain2Pin);
    releasePin(pins.bin1Pin);
    releasePin(pins.bin2Pin);
  }

  bool configure(const DRV8704Pins&,
                 const DRV8704PwmConfig& config,
                 DRV8704PwmCapability& capability) override {
    capability.available = true;
    capability.backendType = PwmBackendType::GeneralAnalogWrite;
    capability.requestedFrequencyHz = config.frequencyHz;
    capability.achievedFrequencyHz = 0UL;
    capability.resolutionBits = kGeneralResolutionBits;
    capability.dutySteps = kGeneralDutySteps;
    capability.smallestDutyIncrementPercent = 100.0f / 255.0f;
    return true;
  }

  bool writeDuty(int8_t pin, uint32_t dutyCount, uint32_t maxDuty) override {
    if (pin < 0) {
      return false;
    }

    if (maxDuty == 0UL) {
      analogWrite(static_cast<uint8_t>(pin), 0);
      return true;
    }

    if (dutyCount > maxDuty) {
      dutyCount = maxDuty;
    }

    const uint32_t scaled = (dutyCount * 255UL + (maxDuty / 2UL)) / maxDuty;
    analogWrite(static_cast<uint8_t>(pin), static_cast<int>(scaled));
    return true;
  }

  void releasePin(int8_t pin) override {
    if (pin < 0) {
      return;
    }
    analogWrite(static_cast<uint8_t>(pin), 0);
    pinMode(static_cast<uint8_t>(pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pin), LOW);
  }

private:
  void initializePins(const DRV8704Pins& pins) {
    initializePin(pins.ain1Pin);
    initializePin(pins.ain2Pin);
    initializePin(pins.bin1Pin);
    initializePin(pins.bin2Pin);
  }

  void initializePin(int8_t pin) {
    if (pin < 0) {
      return;
    }
    pinMode(static_cast<uint8_t>(pin), OUTPUT);
    analogWrite(static_cast<uint8_t>(pin), 0);
  }
};

} // namespace

DRV8704PwmBackend* drv8704CreatePwmBackend() {
  static GeneralAnalogWriteBackend backend;
  return &backend;
}

#endif
