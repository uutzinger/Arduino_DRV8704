/**
 * @file drv8704_status.cpp
 * @brief Status and fault helpers for the DRV8704 driver.
 */

#include "drv8704.h"

DRV8704Status DRV8704::readStatus() {
  const uint16_t raw = readRegister(DRV8704_REG_STATUS);

  DRV8704Status status;
  status.raw = raw;
  status.overTemperature = (raw & DRV8704_STATUS_OTS_MASK) != 0U;
  status.overCurrentA = (raw & DRV8704_STATUS_AOCP_MASK) != 0U;
  status.overCurrentB = (raw & DRV8704_STATUS_BOCP_MASK) != 0U;
  status.predriverFaultA = (raw & DRV8704_STATUS_APDF_MASK) != 0U;
  status.predriverFaultB = (raw & DRV8704_STATUS_BPDF_MASK) != 0U;
  status.undervoltage = (raw & DRV8704_STATUS_UVLO_MASK) != 0U;
  return status;
}

bool DRV8704::isFaultPinActive() const {
  if (!hasFaultPin()) {
    return false;
  }

  return digitalRead(static_cast<uint8_t>(pins_.faultPin)) == LOW;
}

bool DRV8704::hasFault() {
  const DRV8704Status status = readStatus();
  return isFaultPinActive() ||
         status.overTemperature ||
         status.overCurrentA ||
         status.overCurrentB ||
         status.predriverFaultA ||
         status.predriverFaultB ||
         status.undervoltage;
}

bool DRV8704::clearFaults() {
  return writeRegister(DRV8704_REG_STATUS, 0x0000U);
}

bool DRV8704::clearFault(FaultBit bit) {
  const uint16_t mask = static_cast<uint16_t>(1U << static_cast<uint8_t>(bit)) & DRV8704_STATUS_FAULT_MASK;
  return updateRegister(DRV8704_REG_STATUS, mask, 0x0000U);
}
