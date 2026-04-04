/**
 * @file drv8704_pwm_teensy.cpp
 * @brief Teensy hardware-PWM backend for DRV8704 PWM input generation.
 */

#include "drv8704_pwm.h"

#include "drv8704_defs.h"

#if defined(CORE_TEENSY)

namespace {

uint32_t clampFrequency(uint32_t frequencyHz) {
  if (frequencyHz < DRV8704_MIN_INPUT_PWM_HZ) {
    return DRV8704_MIN_INPUT_PWM_HZ;
  }
  if (frequencyHz > DRV8704_MAX_INPUT_PWM_HZ) {
    return DRV8704_MAX_INPUT_PWM_HZ;
  }
  return frequencyHz;
}

uint8_t clampResolution(uint8_t bits) {
  if (bits < 1U) {
    return 1U;
  }
  if (bits > DRV8704_MAX_PWM_RES_BITS) {
    return DRV8704_MAX_PWM_RES_BITS;
  }
  return bits;
}

uint32_t maxDutyFromBits(uint8_t bits) {
  if (bits == 0U) {
    return 0UL;
  }
  if (bits >= 31U) {
    return 0x7FFFFFFFUL;
  }
  return (1UL << bits) - 1UL;
}

class TeensyPwmBackend : public DRV8704PwmBackend {
public:
  TeensyPwmBackend() : frequencyHz_(0UL), resolutionBits_(0U) {}

  bool begin(const DRV8704Pins& pins,
             const DRV8704PwmConfig& config,
             DRV8704PwmCapability& capability) override {
    return configure(pins, config, capability);
  }

  void end(const DRV8704Pins& pins) override {
    releasePin(pins.ain1Pin);
    releasePin(pins.ain2Pin);
    releasePin(pins.bin1Pin);
    releasePin(pins.bin2Pin);
  }

  bool configure(const DRV8704Pins& pins,
                 const DRV8704PwmConfig& config,
                 DRV8704PwmCapability& capability) override {
    frequencyHz_ = clampFrequency(config.frequencyHz);
    resolutionBits_ = clampResolution(config.preferredResolutionBits);

    analogWriteResolution(resolutionBits_);
    configurePin(pins.ain1Pin);
    configurePin(pins.ain2Pin);
    configurePin(pins.bin1Pin);
    configurePin(pins.bin2Pin);

    capability.available = true;
    capability.backendType = PwmBackendType::TeensyHardware;
    capability.requestedFrequencyHz = config.frequencyHz;
    capability.achievedFrequencyHz = frequencyHz_;
    capability.resolutionBits = resolutionBits_;
    capability.dutySteps = maxDutyFromBits(resolutionBits_) + 1UL;
    capability.smallestDutyIncrementPercent =
        (capability.dutySteps > 1UL) ? (100.0f / static_cast<float>(capability.dutySteps - 1UL)) : 100.0f;
    return true;
  }

  bool writeDuty(int8_t pin, uint32_t dutyCount, uint32_t maxDuty) override {
    if (pin < 0) {
      return false;
    }
    if (dutyCount > maxDuty) {
      dutyCount = maxDuty;
    }
    analogWriteResolution(resolutionBits_);
    analogWrite(static_cast<uint8_t>(pin), dutyCount);
    return true;
  }

  void releasePin(int8_t pin) override {
    if (pin < 0) {
      return;
    }
    pinMode(static_cast<uint8_t>(pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pin), LOW);
  }

private:
  void configurePin(int8_t pin) {
    if (pin < 0) {
      return;
    }
    pinMode(static_cast<uint8_t>(pin), OUTPUT);
    analogWriteFrequency(static_cast<uint8_t>(pin), frequencyHz_);
    analogWrite(static_cast<uint8_t>(pin), 0U);
  }

  uint32_t frequencyHz_;
  uint8_t resolutionBits_;
};

} // namespace

DRV8704PwmBackend* drv8704CreatePwmBackend() {
  static TeensyPwmBackend backend;
  return &backend;
}

#endif // CORE_TEENSY
