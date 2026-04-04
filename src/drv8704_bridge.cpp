/**
 * @file drv8704_bridge.cpp
 * @brief Runtime bridge GPIO helpers for the DRV8704 driver.
 */

#include "drv8704.h"
#include "drv8704_pwm.h"

namespace {

uint8_t bridgeIndex(BridgeId bridge) {
  return (bridge == BridgeId::A) ? 0U : 1U;
}

} // namespace

void DRV8704::setBridgePins(int8_t in1Pin, int8_t in2Pin, BridgeMode mode) {
  if (in1Pin < 0 || in2Pin < 0) {
    return;
  }

  switch (mode) {
    case BridgeMode::Coast:
      digitalWrite(static_cast<uint8_t>(in1Pin), LOW);
      digitalWrite(static_cast<uint8_t>(in2Pin), LOW);
      break;
    case BridgeMode::Forward:
      digitalWrite(static_cast<uint8_t>(in1Pin), HIGH);
      digitalWrite(static_cast<uint8_t>(in2Pin), LOW);
      break;
    case BridgeMode::Reverse:
      digitalWrite(static_cast<uint8_t>(in1Pin), LOW);
      digitalWrite(static_cast<uint8_t>(in2Pin), HIGH);
      break;
    case BridgeMode::Brake:
      digitalWrite(static_cast<uint8_t>(in1Pin), HIGH);
      digitalWrite(static_cast<uint8_t>(in2Pin), HIGH);
      break;
  }
}

void DRV8704::setBridgeStaticMode(BridgeId bridge, BridgeMode mode) {
  if (pwmModeEnabled_ && pwmBackend_ != nullptr) {
    if (bridge == BridgeId::A) {
      pwmBackend_->releasePin(pins_.ain1Pin);
      pwmBackend_->releasePin(pins_.ain2Pin);
    } else {
      pwmBackend_->releasePin(pins_.bin1Pin);
      pwmBackend_->releasePin(pins_.bin2Pin);
    }
  }

  if (bridge == BridgeId::A) {
    setBridgePins(pins_.ain1Pin, pins_.ain2Pin, mode);
  } else {
    setBridgePins(pins_.bin1Pin, pins_.bin2Pin, mode);
  }
}

bool DRV8704::setBridgePwm(int8_t in1Pin, int8_t in2Pin, Direction direction, float speedPercent) {
  if (!pwmModeEnabled_ || pwmBackend_ == nullptr || in1Pin < 0 || in2Pin < 0) {
    return false;
  }

  if (speedPercent <= 0.0f) {
    pwmBackend_->releasePin(in1Pin);
    pwmBackend_->releasePin(in2Pin);
    digitalWrite(static_cast<uint8_t>(in1Pin), LOW);
    digitalWrite(static_cast<uint8_t>(in2Pin), LOW);
    return true;
  }

  if (speedPercent >= 100.0f) {
    pwmBackend_->releasePin(in1Pin);
    pwmBackend_->releasePin(in2Pin);
    if (direction == Direction::Forward) {
      digitalWrite(static_cast<uint8_t>(in1Pin), HIGH);
      digitalWrite(static_cast<uint8_t>(in2Pin), LOW);
    } else {
      digitalWrite(static_cast<uint8_t>(in1Pin), LOW);
      digitalWrite(static_cast<uint8_t>(in2Pin), HIGH);
    }
    return true;
  }

  const uint32_t maxDuty = (pwmCapability_.dutySteps > 1UL) ? (pwmCapability_.dutySteps - 1UL) : 0UL;
  if (maxDuty == 0UL) {
    return false;
  }

  uint32_t dutyCount = static_cast<uint32_t>(
      (speedPercent * static_cast<float>(maxDuty) / 100.0f) + 0.5f);
  if (dutyCount == 0UL) {
    dutyCount = 1UL;
  }
  if (dutyCount >= maxDuty) {
    dutyCount = maxDuty - 1UL;
  }

  const int8_t pwmPin = (direction == Direction::Forward) ? in1Pin : in2Pin;
  const int8_t staticLowPin = (direction == Direction::Forward) ? in2Pin : in1Pin;

  pwmBackend_->releasePin(staticLowPin);
  digitalWrite(static_cast<uint8_t>(staticLowPin), LOW);
  return pwmBackend_->writeDuty(pwmPin, dutyCount, maxDuty);
}

bool DRV8704::applyBridgeState(BridgeId bridge) {
  if (!initialized_) {
    return false;
  }

  const uint8_t index = bridgeIndex(bridge);
  switch (bridgeStates_[index].runtimeState) {
    case BridgeRuntimeState::Coast:
      setBridgeStaticMode(bridge, BridgeMode::Coast);
      return true;
    case BridgeRuntimeState::Brake:
      setBridgeStaticMode(bridge, BridgeMode::Brake);
      return true;
    case BridgeRuntimeState::CurrentDrive:
      setBridgeStaticMode(
          bridge,
          (bridgeDirections_[index] == Direction::Forward) ? BridgeMode::Forward : BridgeMode::Reverse);
      return true;
    case BridgeRuntimeState::PwmDriveWithCurrentLimit:
      if (!currentLimitEnabled_) {
        return false;
      }
      if (bridge == BridgeId::A) {
        return setBridgePwm(pins_.ain1Pin, pins_.ain2Pin, bridgeDirections_[index], bridgeSpeedPercents_[index]);
      }
      return setBridgePwm(pins_.bin1Pin, pins_.bin2Pin, bridgeDirections_[index], bridgeSpeedPercents_[index]);
  }

  return false;
}

void DRV8704::coast(BridgeId bridge) {
  const uint8_t index = bridgeIndex(bridge);
  bridgeStates_[index].runtimeState = BridgeRuntimeState::Coast;
  bridgeStates_[index].speedPercent = 0.0f;
  bridgeSpeedPercents_[index] = 0.0f;
  if (initialized_) {
    setBridgeStaticMode(bridge, BridgeMode::Coast);
  }
}

void DRV8704::brake(BridgeId bridge) {
  const uint8_t index = bridgeIndex(bridge);
  bridgeStates_[index].runtimeState = BridgeRuntimeState::Brake;
  bridgeStates_[index].speedPercent = 0.0f;
  bridgeSpeedPercents_[index] = 0.0f;
  if (initialized_) {
    setBridgeStaticMode(bridge, BridgeMode::Brake);
  }
}

void DRV8704::setBridgeMode(BridgeId bridge, BridgeMode mode) {
  switch (mode) {
    case BridgeMode::Coast:
      coast(bridge);
      break;
    case BridgeMode::Brake:
      brake(bridge);
      break;
    case BridgeMode::Forward:
      setDirection(bridge, Direction::Forward);
      bridgeStates_[bridgeIndex(bridge)].runtimeState = BridgeRuntimeState::CurrentDrive;
      applyBridgeState(bridge);
      break;
    case BridgeMode::Reverse:
      setDirection(bridge, Direction::Reverse);
      bridgeStates_[bridgeIndex(bridge)].runtimeState = BridgeRuntimeState::CurrentDrive;
      applyBridgeState(bridge);
      break;
  }
}

DRV8704BridgeState DRV8704::bridgeState(BridgeId bridge) const {
  return bridgeStates_[bridgeIndex(bridge)];
}

bool DRV8704::beginPwmMode(const DRV8704PwmConfig& config) {
  if (!initialized_) {
    return false;
  }

  pwmConfig_ = config;
  if (pwmConfig_.frequencyHz < DRV8704_MIN_INPUT_PWM_HZ) {
    pwmConfig_.frequencyHz = DRV8704_MIN_INPUT_PWM_HZ;
  } else if (pwmConfig_.frequencyHz > DRV8704_MAX_INPUT_PWM_HZ) {
    pwmConfig_.frequencyHz = DRV8704_MAX_INPUT_PWM_HZ;
  }

  if (pwmConfig_.preferredResolutionBits < 1U) {
    pwmConfig_.preferredResolutionBits = 1U;
  } else if (pwmConfig_.preferredResolutionBits > DRV8704_MAX_PWM_RES_BITS) {
    pwmConfig_.preferredResolutionBits = DRV8704_MAX_PWM_RES_BITS;
  }

  if (pwmBackend_ == nullptr) {
    pwmBackend_ = drv8704CreatePwmBackend();
  }

  pwmCapability_ = DRV8704PwmCapability();
  if (pwmBackend_ == nullptr) {
    return false;
  }

  pwmModeEnabled_ = pwmBackend_->begin(pins_, pwmConfig_, pwmCapability_);
  if (!pwmModeEnabled_) {
    pwmCapability_.backendType = PwmBackendType::Unsupported;
    return false;
  }

  if (currentLimitEnabled_ && currentLimitResult_.valid) {
    return applyCurrentLimitResult(currentLimitResult_, pwmConfig_.frequencyHz);
  }

  return true;
}

void DRV8704::endPwmMode() {
  if (pwmBackend_ != nullptr && pwmModeEnabled_) {
    pwmBackend_->end(pins_);
  }

  if (pins_.ain1Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.ain1Pin), OUTPUT);
  }
  if (pins_.ain2Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.ain2Pin), OUTPUT);
  }
  if (pins_.bin1Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.bin1Pin), OUTPUT);
  }
  if (pins_.bin2Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.bin2Pin), OUTPUT);
  }

  pwmCapability_ = DRV8704PwmCapability();
  pwmModeEnabled_ = false;

  for (uint8_t i = 0; i < 2U; ++i) {
    if (bridgeStates_[i].runtimeState == BridgeRuntimeState::PwmDriveWithCurrentLimit) {
      bridgeStates_[i].runtimeState = BridgeRuntimeState::Coast;
      bridgeStates_[i].speedPercent = 0.0f;
      bridgeSpeedPercents_[i] = 0.0f;
    }
  }

  setBridgePins(pins_.ain1Pin, pins_.ain2Pin, BridgeMode::Coast);
  setBridgePins(pins_.bin1Pin, pins_.bin2Pin, BridgeMode::Coast);
}

bool DRV8704::setPwmFrequency(uint32_t frequencyHz) {
  if (!pwmModeEnabled_ || pwmBackend_ == nullptr) {
    return false;
  }

  pwmConfig_.frequencyHz = frequencyHz;
  if (!pwmBackend_->configure(pins_, pwmConfig_, pwmCapability_)) {
    return false;
  }

  if (currentLimitEnabled_ && currentLimitResult_.valid) {
    return applyCurrentLimitResult(currentLimitResult_, pwmCapability_.achievedFrequencyHz);
  }

  return true;
}

bool DRV8704::setSpeed(BridgeId bridge, float speedPercent) {
  const uint8_t index = bridgeIndex(bridge);
  if (speedPercent <= 0.0f) {
    coast(bridge);
    return true;
  }

  if (!pwmModeEnabled_ || pwmBackend_ == nullptr || !currentLimitEnabled_) {
    return false;
  }

  bridgeStates_[index].runtimeState = BridgeRuntimeState::PwmDriveWithCurrentLimit;
  bridgeStates_[index].direction = bridgeDirections_[index];
  if (speedPercent > 100.0f) {
    speedPercent = 100.0f;
  }
  bridgeStates_[index].speedPercent = speedPercent;
  bridgeSpeedPercents_[index] = speedPercent;
  return applyBridgeState(bridge);
}
