/**
 * @file drv8704_pwm_esp32.cpp
 * @brief ESP32 LEDC backend for DRV8704 PWM input generation.
 */

#include "drv8704_pwm.h"

#include "drv8704_defs.h"

#if defined(ARDUINO_ARCH_ESP32)

#if defined(__has_include)
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#endif

namespace {

static const uint8_t kDrv8704Esp32Channels[4] = {0U, 1U, 2U, 3U};

uint32_t clampFrequency(uint32_t frequencyHz) {
  if (frequencyHz < DRV8704_MIN_INPUT_PWM_HZ) {
    return DRV8704_MIN_INPUT_PWM_HZ;
  }
  if (frequencyHz > DRV8704_MAX_INPUT_PWM_HZ) {
    return DRV8704_MAX_INPUT_PWM_HZ;
  }
  return frequencyHz;
}

uint8_t resolveResolutionBits(uint32_t frequencyHz, uint8_t preferredBits) {
  const uint32_t ledcClockHz = 80000000UL;
  uint8_t bits = preferredBits;

  if (bits < 1U) {
    bits = 1U;
  }
  if (bits > DRV8704_MAX_PWM_RES_BITS) {
    bits = DRV8704_MAX_PWM_RES_BITS;
  }

  while (bits > 1U) {
    const uint32_t maxFrequencyAtBits = ledcClockHz / (1UL << bits);
    if (maxFrequencyAtBits >= frequencyHz) {
      break;
    }
    --bits;
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

class Esp32LedcBackend : public DRV8704PwmBackend {
public:
  Esp32LedcBackend()
      : resolutionBits_(0U),
        frequencyHz_(0UL),
        assignedPins_{-1, -1, -1, -1},
        attached_{false, false, false, false} {}

  bool begin(const DRV8704Pins& pins,
             const DRV8704PwmConfig& config,
             DRV8704PwmCapability& capability) override {
    return configure(pins, config, capability);
  }

  void end(const DRV8704Pins& pins) override {
    releasePins(pins);
    frequencyHz_ = 0UL;
    resolutionBits_ = 0U;
  }

  bool configure(const DRV8704Pins& pins,
                 const DRV8704PwmConfig& config,
                 DRV8704PwmCapability& capability) override {
    const uint32_t clampedFrequency = clampFrequency(config.frequencyHz);
    const uint8_t resolvedBits = resolveResolutionBits(clampedFrequency, config.preferredResolutionBits);

    releasePins(pins);
    frequencyHz_ = clampedFrequency;
    resolutionBits_ = resolvedBits;
    assignedPins_[0] = pins.ain1Pin;
    assignedPins_[1] = pins.ain2Pin;
    assignedPins_[2] = pins.bin1Pin;
    assignedPins_[3] = pins.bin2Pin;

    capability.available = true;
    capability.backendType = PwmBackendType::Esp32Ledc;
    capability.requestedFrequencyHz = config.frequencyHz;
    capability.achievedFrequencyHz = clampedFrequency;
    capability.resolutionBits = resolvedBits;
    capability.dutySteps = maxDutyFromBits(resolvedBits) + 1UL;
    capability.smallestDutyIncrementPercent =
        (capability.dutySteps > 1UL) ? (100.0f / static_cast<float>(capability.dutySteps - 1UL)) : 100.0f;
    return true;
  }

  bool writeDuty(int8_t pin, uint32_t dutyCount, uint32_t maxDuty) override {
    const int8_t slot = slotForPin(pin);
    if (slot < 0) {
      return false;
    }

    attachPinIfNeeded(pin, static_cast<uint8_t>(slot));
    if (!attached_[slot]) {
      return false;
    }

    if (dutyCount > maxDuty) {
      dutyCount = maxDuty;
    }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    return ledcWrite(static_cast<uint8_t>(pin), dutyCount);
#else
    ledcWrite(kDrv8704Esp32Channels[slot], dutyCount);
    return true;
#endif
  }

  void releasePin(int8_t pin) override {
    const int8_t slot = slotForPin(pin);
    if (slot < 0 || !attached_[slot]) {
      return;
    }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcDetach(static_cast<uint8_t>(pin));
#else
    ledcDetachPin(static_cast<uint8_t>(pin));
#endif
    attached_[slot] = false;
    pinMode(static_cast<uint8_t>(pin), OUTPUT);
  }

private:
  int8_t slotForPin(int8_t pin) const {
    if (pin < 0) {
      return -1;
    }
    for (uint8_t i = 0; i < 4U; ++i) {
      if (assignedPins_[i] == pin) {
        return static_cast<int8_t>(i);
      }
    }
    return -1;
  }

  void releasePins(const DRV8704Pins& pins) {
    releasePin(pins.ain1Pin);
    releasePin(pins.ain2Pin);
    releasePin(pins.bin1Pin);
    releasePin(pins.bin2Pin);
  }

  void attachPinIfNeeded(int8_t pin, uint8_t slot) {
    if (attached_[slot]) {
      return;
    }

    pinMode(static_cast<uint8_t>(pin), OUTPUT);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    attached_[slot] = ledcAttach(static_cast<uint8_t>(pin), frequencyHz_, resolutionBits_);
#else
    attached_[slot] = (ledcSetup(kDrv8704Esp32Channels[slot], frequencyHz_, resolutionBits_) > 0.0);
    if (attached_[slot]) {
      ledcAttachPin(static_cast<uint8_t>(pin), kDrv8704Esp32Channels[slot]);
    }
#endif
  }

  uint8_t resolutionBits_;
  uint32_t frequencyHz_;
  int8_t assignedPins_[4];
  bool attached_[4];
};

} // namespace

DRV8704PwmBackend* drv8704CreatePwmBackend() {
  static Esp32LedcBackend backend;
  return &backend;
}

#endif // ARDUINO_ARCH_ESP32
