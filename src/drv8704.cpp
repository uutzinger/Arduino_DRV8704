/**
 * @file drv8704.cpp
 * @brief Lifecycle and top-level orchestration for the DRV8704 driver.
 */

#include "drv8704.h"

DRV8704::DRV8704(const DRV8704Pins& pins, SPIClass& spi)
    : pins_(pins),
      spi_(&spi),
      spiSettings_(DRV8704_SPI_SPEED, DRV8704_SPI_ORDER, DRV8704_SPI_MODE),
      registerCache_{0},
      shuntOhms_{0.0005f, 0.0005f},
      bridgeDirections_{Direction::Forward, Direction::Forward},
      bridgeStates_{},
      bridgeSpeedPercents_{0.0f, 0.0f},
      currentPreset_(CurrentModePreset::MotorMediumInductance),
      currentPresetConfig_(),
      currentLimitResult_(),
      currentLimitEnabled_(false),
      pwmBackend_(nullptr),
      pwmConfig_(),
      pwmCapability_(),
      pwmModeEnabled_(false),
      initialized_(false) {
  seedDefaultRegisterCache();
  currentPresetConfig_ = presetConfig(currentPreset_, 0UL);
  bridgeStates_[0].bridge = BridgeId::A;
  bridgeStates_[0].direction = Direction::Forward;
  bridgeStates_[1].bridge = BridgeId::B;
  bridgeStates_[1].direction = Direction::Forward;
}

bool DRV8704::begin() {
  endPwmMode();
  initialized_ = false;
  seedDefaultRegisterCache();

  pinMode(pins_.csPin, OUTPUT);
  digitalWrite(pins_.csPin, LOW);

  if (pins_.sleepPin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.sleepPin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pins_.sleepPin), LOW);
  }

  if (pins_.resetPin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.resetPin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pins_.resetPin), LOW);
  }

  if (pins_.faultPin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.faultPin), INPUT_PULLUP);
  }

  if (pins_.ain1Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.ain1Pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pins_.ain1Pin), LOW);
  }
  if (pins_.ain2Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.ain2Pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pins_.ain2Pin), LOW);
  }
  if (pins_.bin1Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.bin1Pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pins_.bin1Pin), LOW);
  }
  if (pins_.bin2Pin >= 0) {
    pinMode(static_cast<uint8_t>(pins_.bin2Pin), OUTPUT);
    digitalWrite(static_cast<uint8_t>(pins_.bin2Pin), LOW);
  }

  spi_->begin();

  wake();

  if (pins_.resetPin >= 0) {
    reset();
  } else {
    delayMicroseconds(DRV8704_RESET_RECOVERY_US);
  }

  initialized_ = true;
  if (!syncRegisterCache()) {
    initialized_ = false;
    return false;
  }

  const DRV8704HealthCheck health = healthCheck();
  if (!health.spiOk) {
    initialized_ = false;
    return false;
  }

  return true;
}

void DRV8704::end() {
  endPwmMode();
  initialized_ = false;
}

void DRV8704::sleep() {
  if (pins_.sleepPin >= 0) {
    digitalWrite(static_cast<uint8_t>(pins_.sleepPin), LOW);
  }
}

void DRV8704::wake() {
  if (pins_.sleepPin >= 0) {
    digitalWrite(static_cast<uint8_t>(pins_.sleepPin), HIGH);
    delayMicroseconds(DRV8704_WAKE_DELAY_US);
  }
}

void DRV8704::reset() {
  if (pins_.resetPin < 0) {
    return;
  }

  digitalWrite(static_cast<uint8_t>(pins_.resetPin), HIGH);
  delayMicroseconds(DRV8704_RESET_PULSE_WIDTH_US);
  digitalWrite(static_cast<uint8_t>(pins_.resetPin), LOW);
  delayMicroseconds(DRV8704_RESET_RECOVERY_US);
}

DRV8704HealthCheck DRV8704::healthCheck() {
  DRV8704HealthCheck result;
  result.initialized = initialized_;
  result.faultPinActive = isFaultPinActive();
  if (!initialized_) {
    return result;
  }

  result.defaultsMatch = checkDefaultRegisters(result.defaultMatches);
  result.writeReadbackOk = performWriteReadbackProbe();
  result.statusRegister = readRegister(DRV8704_REG_STATUS);
  result.faultPresent =
      result.faultPinActive || ((result.statusRegister & DRV8704_STATUS_FAULT_MASK) != 0U);
  result.spiOk = result.defaultsMatch && result.writeReadbackOk;
  return result;
}

uint16_t DRV8704::cachedRegister(uint8_t address) const {
  if (!isValidAddress(address)) {
    return 0U;
  }

  return registerCache_[address];
}

bool DRV8704::syncRegisterCache() {
  if (!initialized_) {
    return false;
  }

  for (uint8_t address = 0; address < DRV8704_REGISTER_COUNT; ++address) {
    registerCache_[address] = readRegister(address);
  }

  return true;
}

bool DRV8704::isValidAddress(uint8_t address) const {
  return address < DRV8704_REGISTER_COUNT;
}

void DRV8704::seedDefaultRegisterCache() {
  for (uint8_t i = 0; i < DRV8704_REGISTER_COUNT; ++i) {
    registerCache_[i] = 0U;
  }

  registerCache_[DRV8704_REG_CTRL] = DRV8704_CTRL_DEFAULT;
  registerCache_[DRV8704_REG_TORQUE] = DRV8704_TORQUE_DEFAULT;
  registerCache_[DRV8704_REG_OFF] = DRV8704_OFF_DEFAULT;
  registerCache_[DRV8704_REG_BLANK] = DRV8704_BLANK_DEFAULT;
  registerCache_[DRV8704_REG_DECAY] = DRV8704_DECAY_DEFAULT;
  registerCache_[DRV8704_REG_RESERVED] = 0U;
  registerCache_[DRV8704_REG_DRIVE] = DRV8704_DRIVE_DEFAULT;
  registerCache_[DRV8704_REG_STATUS] = DRV8704_STATUS_DEFAULT;
}

bool DRV8704::checkDefaultRegisters(uint8_t& matchCount) {
  matchCount = 0;

  struct ExpectedRegister {
    uint8_t address;
    uint16_t expected;
  };

  static const ExpectedRegister expectedRegisters[] = {
      {DRV8704_REG_CTRL, DRV8704_CTRL_DEFAULT},
      {DRV8704_REG_TORQUE, DRV8704_TORQUE_DEFAULT},
      {DRV8704_REG_OFF, DRV8704_OFF_DEFAULT},
      {DRV8704_REG_BLANK, DRV8704_BLANK_DEFAULT},
      {DRV8704_REG_DECAY, DRV8704_DECAY_DEFAULT},
      {DRV8704_REG_DRIVE, DRV8704_DRIVE_DEFAULT},
  };

  const uint8_t registerCount = sizeof(expectedRegisters) / sizeof(expectedRegisters[0]);
  for (uint8_t i = 0; i < registerCount; ++i) {
    const uint16_t value = readRegister(expectedRegisters[i].address);
    if (value == expectedRegisters[i].expected) {
      ++matchCount;
    }
  }

  return matchCount == registerCount;
}

bool DRV8704::performWriteReadbackProbe() {
  const uint16_t originalTorque = readRegister(DRV8704_REG_TORQUE) & DRV8704_TORQUE_MASK;
  uint16_t probeTorque = static_cast<uint16_t>((originalTorque ^ 0x0055U) & DRV8704_TORQUE_MASK);

  if (probeTorque == originalTorque) {
    probeTorque = static_cast<uint16_t>((originalTorque + 1U) & DRV8704_TORQUE_MASK);
  }

  const bool writeOk = writeRegister(DRV8704_REG_TORQUE, probeTorque);
  const uint16_t readback = readRegister(DRV8704_REG_TORQUE) & DRV8704_TORQUE_MASK;
  const bool probeOk = writeOk && (readback == probeTorque);

  const bool restoreOk = writeRegister(DRV8704_REG_TORQUE, originalTorque);
  const uint16_t restored = readRegister(DRV8704_REG_TORQUE) & DRV8704_TORQUE_MASK;

  return probeOk && restoreOk && (restored == originalTorque);
}

void DRV8704::beginTransaction() {
  spi_->beginTransaction(spiSettings_);
  digitalWrite(pins_.csPin, HIGH);
}

void DRV8704::endTransaction() {
  digitalWrite(pins_.csPin, LOW);
  spi_->endTransaction();
}

uint16_t DRV8704::transferFrame(uint16_t frame) {
  beginTransaction();
  const uint16_t response = spi_->transfer16(frame);
  endTransaction();
  return response;
}
