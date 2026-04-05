/**
 * @file drv8704_comm.cpp
 * @brief SPI transport helpers for the DRV8704 driver.
 */

#include "drv8704.h"
#include <logger.h>

uint16_t DRV8704::readRegister(uint8_t address) {
  if (!initialized_) {
    LOGE("DRV8704 readRegister: device not initialized (address=0x%02X)", address);
    return 0U;
  }

  if (!isValidAddress(address)) {
    LOGE("DRV8704 readRegister: invalid address 0x%02X", address);
    return 0U;
  }

  const uint16_t frame =
      DRV8704_READ_BIT |
      (static_cast<uint16_t>(address & DRV8704_ADDRESS_MASK) << DRV8704_ADDRESS_SHIFT);
  const uint16_t response = transferFrame(frame);
  const uint16_t value = response & DRV8704_REGISTER_MASK;

  if (address < DRV8704_REGISTER_COUNT) {
    registerCache_[address] = value;
  }

  return value;
}

bool DRV8704::writeRegister(uint8_t address, uint16_t value) {
  if (!initialized_) {
    LOGE("DRV8704 writeRegister: device not initialized (address=0x%02X, value=0x%04X)",
         address,
         value);
    return false;
  }

  if (!isValidAddress(address)) {
    LOGE("DRV8704 writeRegister: invalid address 0x%02X", address);
    return false;
  }

  const uint16_t payload = value & DRV8704_REGISTER_MASK;
  const uint16_t frame =
      DRV8704_WRITE_BIT |
      (static_cast<uint16_t>(address & DRV8704_ADDRESS_MASK) << DRV8704_ADDRESS_SHIFT) |
      payload;

  (void)transferFrame(frame);

  if (address < DRV8704_REGISTER_COUNT) {
    registerCache_[address] = payload;
  }

  return true;
}

bool DRV8704::updateRegister(uint8_t address, uint16_t mask, uint16_t value) {
  if (!initialized_) {
    LOGE("DRV8704 updateRegister: device not initialized (address=0x%02X)", address);
    return false;
  }

  if (!isValidAddress(address)) {
    LOGE("DRV8704 updateRegister: invalid address 0x%02X", address);
    return false;
  }

  const uint16_t current = readRegister(address);
  const uint16_t next = static_cast<uint16_t>((current & ~mask) | (value & mask));
  return writeRegister(address, next);
}
