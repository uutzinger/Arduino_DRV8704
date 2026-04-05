/**
 * @file drv8704_current.cpp
 * @brief Current-limit and current-drive helpers for the DRV8704 driver.
 */

#include "drv8704.h"
#include <logger.h>

namespace {

static const float kDrv8704VrefFullScale = 2.75f;
static const uint16_t kDrv8704TorqueMax = 0xFFU;
static const float kDrv8704MinimumShuntOhms = 1.0e-6f;

uint8_t bridgeIndex(BridgeId bridge) {
  return (bridge == BridgeId::A) ? 0U : 1U;
}

} // namespace

float DRV8704::senseGainValue(SenseGain gain) const {
  switch (gain) {
    case SenseGain::Gain5VperV:
      return 5.0f;
    case SenseGain::Gain10VperV:
      return 10.0f;
    case SenseGain::Gain20VperV:
      return 20.0f;
    case SenseGain::Gain40VperV:
      return 40.0f;
  }
  return 5.0f;
}

float DRV8704::shuntResistance(BridgeId bridge) const {
  return shuntOhms_[bridgeIndex(bridge)];
}

float DRV8704::currentLimitReferenceShunt() const {
  float reference = shuntOhms_[0];
  if (!(reference > kDrv8704MinimumShuntOhms)) {
    reference = shuntOhms_[1];
  }
  if (!(reference > kDrv8704MinimumShuntOhms)) {
    return 0.0f;
  }

  if ((shuntOhms_[1] > kDrv8704MinimumShuntOhms) && (shuntOhms_[1] < reference)) {
    reference = shuntOhms_[1];
  }
  return reference;
}

DRV8704CurrentPresetConfig DRV8704::presetConfig(CurrentModePreset preset, uint32_t pwmFrequencyHz) const {
  DRV8704CurrentPresetConfig config;
  config.preset = preset;

  switch (preset) {
    case CurrentModePreset::Heater:
      // Bench testing on a resistive heater pad showed the highest delivered current with
      // minimum blanking and auto-mixed decay. A longer off-time still worked well.
      config.offTime = 0x40U;
      config.blankTime = 0x00U;
      config.decayMode = DecayMode::AutoMixed;
      config.decayTime = 0x10U;
      config.deadTime = DeadTime::Ns410;
      break;
    case CurrentModePreset::ThermoelectricCooler:
      // TECs are weakly inductive bipolar loads. Mixed decay is a practical middle ground.
      config.offTime = (pwmFrequencyHz >= 20000UL) ? 0x30U : 0x38U;
      config.blankTime = 0x80U;
      config.decayMode = DecayMode::Mixed;
      config.decayTime = (pwmFrequencyHz >= 20000UL) ? 0x14U : 0x18U;
      config.deadTime = DeadTime::Ns460;
      break;
    case CurrentModePreset::MotorSmallInductance:
      // Low-inductance motors need faster current correction to avoid overshoot.
      config.offTime = (pwmFrequencyHz >= 20000UL) ? 0x18U : 0x20U;
      config.blankTime = (pwmFrequencyHz >= 20000UL) ? 0x30U : 0x40U;
      config.decayMode = DecayMode::Fast;
      config.decayTime = 0x08U;
      config.deadTime = DeadTime::Ns460;
      break;
    case CurrentModePreset::MotorMediumInductance:
      // Medium-inductance motors are the general-purpose default.
      // At higher external PWM frequencies, slightly shorter internal timing reduces layering between
      // input PWM and the DRV8704 fixed-off-time chopper.
      config.offTime = (pwmFrequencyHz >= 20000UL) ? 0x28U : 0x30U;
      config.blankTime = (pwmFrequencyHz >= 20000UL) ? 0x60U : 0x80U;
      config.decayMode = DecayMode::Mixed;
      config.decayTime = (pwmFrequencyHz >= 20000UL) ? 0x0CU : 0x10U;
      config.deadTime = DeadTime::Ns460;
      break;
    case CurrentModePreset::MotorLargeInductance:
      // Higher-inductance motors tolerate longer timing, but high external PWM still benefits
      // from slightly shorter internal timing to avoid sluggish current regulation.
      config.offTime = (pwmFrequencyHz >= 20000UL) ? 0x38U : 0x40U;
      config.blankTime = (pwmFrequencyHz >= 20000UL) ? 0x90U : 0xA0U;
      config.decayMode = DecayMode::AutoMixed;
      config.decayTime = (pwmFrequencyHz >= 20000UL) ? 0x18U : 0x20U;
      config.deadTime = DeadTime::Ns670;
      break;
  }

  return config;
}

bool DRV8704::setShuntResistance(float ohms) {
  return setShuntResistance(BridgeId::A, ohms) && setShuntResistance(BridgeId::B, ohms);
}

bool DRV8704::setShuntResistance(BridgeId bridge, float ohms) {
  if (!(ohms > kDrv8704MinimumShuntOhms)) {
    LOGE("DRV8704 setShuntResistance: invalid shunt value %.9f ohms", ohms);
    return false;
  }

  shuntOhms_[bridgeIndex(bridge)] = ohms;
  return true;
}

bool DRV8704::setCurrentModePreset(CurrentModePreset preset) {
  currentPreset_ = preset;
  currentPresetConfig_ = presetConfig(preset, pwmModeEnabled_ ? pwmConfig_.frequencyHz : 0UL);

  if (!initialized_) {
    return true;
  }

  if (!setDeadTime(currentPresetConfig_.deadTime) ||
      !setOffTime(currentPresetConfig_.offTime) ||
      !setBlankTime(currentPresetConfig_.blankTime) ||
      !setDecayMode(currentPresetConfig_.decayMode) ||
      !setDecayTime(currentPresetConfig_.decayTime)) {
    LOGE("DRV8704 setCurrentModePreset: failed to apply preset timing registers");
    return false;
  }

  if (currentLimitEnabled_ && currentLimitResult_.valid) {
    if (!applyCurrentLimitResult(currentLimitResult_, pwmModeEnabled_ ? pwmConfig_.frequencyHz : 0UL)) {
      LOGE("DRV8704 setCurrentModePreset: failed to reapply current limit after preset change");
      return false;
    }
  }
  return true;
}

bool DRV8704::setDirection(BridgeId bridge, Direction direction) {
  const uint8_t index = bridgeIndex(bridge);
  bridgeDirections_[index] = direction;
  bridgeStates_[index].direction = direction;

  if (!initialized_) {
    return true;
  }

  return applyBridgeState(bridge);
}

Direction DRV8704::direction(BridgeId bridge) const {
  return bridgeDirections_[bridgeIndex(bridge)];
}

bool DRV8704::deriveCurrentLimit(float amps, DRV8704CurrentLimitResult& result) const {
  result = DRV8704CurrentLimitResult();
  result.requestedCurrentLimitAmps = amps;
  result.referenceShuntOhms = currentLimitReferenceShunt();
  result.shuntOhmsA = shuntOhms_[0];
  result.shuntOhmsB = shuntOhms_[1];
  result.preset = currentPreset_;
  result.pwmFrequencyHz = pwmModeEnabled_ ? pwmConfig_.frequencyHz : 0UL;

  if (!(amps > 0.0f) || !(result.referenceShuntOhms > kDrv8704MinimumShuntOhms)) {
    LOGE("DRV8704 deriveCurrentLimit: invalid request (amps=%.6f, reference shunt=%.9f)",
         amps,
         result.referenceShuntOhms);
    return false;
  }

  const SenseGain gainOrder[] = {
      SenseGain::Gain40VperV,
      SenseGain::Gain20VperV,
      SenseGain::Gain10VperV,
      SenseGain::Gain5VperV,
  };

  for (uint8_t i = 0; i < 4U; ++i) {
    const SenseGain gain = gainOrder[i];
    const float gainValue = senseGainValue(gain);
    const float ampsPerCount = kDrv8704VrefFullScale / (256.0f * gainValue * result.referenceShuntOhms);
    const float achievableMax = ampsPerCount * static_cast<float>(kDrv8704TorqueMax);
    const float requestedTorque = amps / ampsPerCount;

    if (requestedTorque <= static_cast<float>(kDrv8704TorqueMax)) {
      uint16_t torqueCode = static_cast<uint16_t>(requestedTorque + 0.5f);
      if (torqueCode == 0U) {
        torqueCode = 1U;
      }
      if (torqueCode > kDrv8704TorqueMax) {
        torqueCode = kDrv8704TorqueMax;
      }

      result.valid = true;
      result.selectedGain = gain;
      result.torqueDac = static_cast<uint8_t>(torqueCode);
      result.ampsPerTorqueCount = ampsPerCount;
      result.achievableMaxCurrentAmps = achievableMax;
      result.appliedCurrentLimitAmpsA =
          (result.shuntOhmsA > kDrv8704MinimumShuntOhms)
              ? (kDrv8704VrefFullScale * static_cast<float>(torqueCode) /
                 (256.0f * gainValue * result.shuntOhmsA))
              : 0.0f;
      result.appliedCurrentLimitAmpsB =
          (result.shuntOhmsB > kDrv8704MinimumShuntOhms)
              ? (kDrv8704VrefFullScale * static_cast<float>(torqueCode) /
                 (256.0f * gainValue * result.shuntOhmsB))
              : 0.0f;
      return true;
    }
  }

  LOGE("DRV8704 deriveCurrentLimit: requested current %.6f A exceeds achievable range", amps);
  return false;
}

bool DRV8704::applyCurrentLimitResult(const DRV8704CurrentLimitResult& result, uint32_t pwmFrequencyHz) {
  const DRV8704CurrentPresetConfig preset = presetConfig(currentPreset_, pwmFrequencyHz);
  if (!setDeadTime(preset.deadTime) ||
      !setOffTime(preset.offTime) ||
      !setBlankTime(preset.blankTime) ||
      !setDecayMode(preset.decayMode) ||
      !setDecayTime(preset.decayTime) ||
      !setSenseGain(result.selectedGain) ||
      !setTorque(result.torqueDac) ||
      !enableBridgeDriver(true)) {
    LOGE("DRV8704 applyCurrentLimitResult: failed to program current-limit registers");
    return false;
  }

  currentPresetConfig_ = preset;
  currentLimitResult_ = result;
  currentLimitResult_.enabled = true;
  currentLimitResult_.pwmFrequencyHz = pwmFrequencyHz;
  currentLimitEnabled_ = true;
  return true;
}

bool DRV8704::setCurrentLimit(float amps) {
  if (!initialized_) {
    LOGE("DRV8704 setCurrentLimit: device not initialized");
    return false;
  }

  DRV8704CurrentLimitResult result;
  if (!deriveCurrentLimit(amps, result)) {
    LOGE("DRV8704 setCurrentLimit: could not derive current-limit settings for %.6f A", amps);
    return false;
  }

  return applyCurrentLimitResult(result, pwmModeEnabled_ ? pwmConfig_.frequencyHz : 0UL);
}

bool DRV8704::setCurrent(BridgeId bridge, float amps) {
  const uint8_t index = bridgeIndex(bridge);
  if (!setCurrentLimit(amps)) {
    LOGE("DRV8704 setCurrent: failed to arm current limit for %.6f A", amps);
    return false;
  }

  bridgeStates_[index].runtimeState = BridgeRuntimeState::CurrentDrive;
  bridgeStates_[index].direction = bridgeDirections_[index];
  bridgeStates_[index].speedPercent = 100.0f;
  bridgeSpeedPercents_[index] = 100.0f;
  if (!applyBridgeState(bridge)) {
    LOGE("DRV8704 setCurrent: failed to apply bridge state");
    return false;
  }
  return true;
}

bool DRV8704::disableCurrentLimit() {
  if (initialized_) {
    if (!setSenseGain(SenseGain::Gain5VperV) || !setTorque(static_cast<uint8_t>(kDrv8704TorqueMax))) {
      LOGE("DRV8704 disableCurrentLimit: failed to restore permissive current-limit settings");
      return false;
    }
  }

  currentLimitEnabled_ = false;
  currentLimitResult_ = DRV8704CurrentLimitResult();
  return true;
}
