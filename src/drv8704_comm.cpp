/**
 * @file drv8704_comm.cpp
 * @brief SPI transport helpers for the DRV8704 driver.
 */

#include "drv8704.h"

uint16_t DRV8704::readRegister(uint8_t address) {
  if (!initialized_ || !isValidAddress(address)) {
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
  if (!initialized_ || !isValidAddress(address)) {
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
  if (!initialized_ || !isValidAddress(address)) {
    return false;
  }

  const uint16_t current = readRegister(address);
  const uint16_t next = static_cast<uint16_t>((current & ~mask) | (value & mask));
  return writeRegister(address, next);
}
