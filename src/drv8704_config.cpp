/**
 * @file drv8704_config.cpp
 * @brief Configuration helpers for the DRV8704 driver.
 */

#include "drv8704.h"

bool DRV8704::writeRegisterVerified(uint8_t address, uint16_t value, uint16_t verifyMask) {
  if (!writeRegister(address, value)) {
    return false;
  }

  const uint16_t readback = readRegister(address);
  return (readback & verifyMask) == (value & verifyMask);
}

bool DRV8704::updateRegisterVerified(uint8_t address, uint16_t mask, uint16_t value, uint16_t verifyMask) {
  const uint16_t current = readRegister(address);
  const uint16_t next = static_cast<uint16_t>((current & ~mask) | (value & mask));
  return writeRegisterVerified(address, next, verifyMask);
}

bool DRV8704::enableBridgeDriver(bool enabled) {
  return updateRegisterVerified(
      DRV8704_REG_CTRL,
      DRV8704_CTRL_ENBL_MASK,
      enabled ? DRV8704_CTRL_ENBL_MASK : 0x0000U,
      DRV8704_CTRL_ENBL_MASK);
}

bool DRV8704::setSenseGain(SenseGain gain) {
  return updateRegisterVerified(
      DRV8704_REG_CTRL,
      DRV8704_CTRL_ISGAIN_MASK,
      static_cast<uint16_t>(static_cast<uint16_t>(gain) << DRV8704_CTRL_ISGAIN_SHIFT),
      DRV8704_CTRL_ISGAIN_MASK);
}

bool DRV8704::setDeadTime(DeadTime deadTime) {
  return updateRegisterVerified(
      DRV8704_REG_CTRL,
      DRV8704_CTRL_DTIME_MASK,
      static_cast<uint16_t>(static_cast<uint16_t>(deadTime) << DRV8704_CTRL_DTIME_SHIFT),
      DRV8704_CTRL_DTIME_MASK);
}

bool DRV8704::setTorque(uint8_t torque) {
  return updateRegisterVerified(
      DRV8704_REG_TORQUE,
      DRV8704_TORQUE_MASK,
      static_cast<uint16_t>(torque) << DRV8704_TORQUE_SHIFT,
      DRV8704_TORQUE_MASK);
}

bool DRV8704::setOffTime(uint8_t offTime) {
  return updateRegisterVerified(
      DRV8704_REG_OFF,
      DRV8704_OFF_TOFF_MASK | DRV8704_OFF_PWMMODE_MASK,
      static_cast<uint16_t>(offTime) << DRV8704_OFF_TOFF_SHIFT |
          DRV8704_OFF_PWMMODE_PWM,
      DRV8704_OFF_TOFF_MASK | DRV8704_OFF_PWMMODE_MASK);
}

bool DRV8704::setBlankTime(uint8_t blankTime) {
  return updateRegisterVerified(
      DRV8704_REG_BLANK,
      DRV8704_BLANK_TBLANK_MASK,
      static_cast<uint16_t>(blankTime) << DRV8704_BLANK_TBLANK_SHIFT,
      DRV8704_BLANK_TBLANK_MASK);
}

bool DRV8704::setDecayMode(DecayMode mode) {
  return updateRegisterVerified(
      DRV8704_REG_DECAY,
      DRV8704_DECAY_DECMOD_MASK,
      static_cast<uint16_t>(static_cast<uint16_t>(mode) << DRV8704_DECAY_DECMOD_SHIFT),
      DRV8704_DECAY_DECMOD_MASK);
}

bool DRV8704::setDecayTime(uint8_t decayTime) {
  return updateRegisterVerified(
      DRV8704_REG_DECAY,
      DRV8704_DECAY_TDECAY_MASK,
      static_cast<uint16_t>(decayTime) << DRV8704_DECAY_TDECAY_SHIFT,
      DRV8704_DECAY_TDECAY_MASK);
}

bool DRV8704::setOcpThreshold(OcpThreshold threshold) {
  return updateRegisterVerified(
      DRV8704_REG_DRIVE,
      DRV8704_DRIVE_OCPTH_MASK,
      static_cast<uint16_t>(static_cast<uint16_t>(threshold) << DRV8704_DRIVE_OCPTH_SHIFT),
      DRV8704_DRIVE_OCPTH_MASK);
}

bool DRV8704::setOcpDeglitch(OcpDeglitch deglitch) {
  return updateRegisterVerified(
      DRV8704_REG_DRIVE,
      DRV8704_DRIVE_OCPDEG_MASK,
      static_cast<uint16_t>(static_cast<uint16_t>(deglitch) << DRV8704_DRIVE_OCPDEG_SHIFT),
      DRV8704_DRIVE_OCPDEG_MASK);
}

bool DRV8704::setGateDriveTimes(GateDriveTime tDriveN, GateDriveTime tDriveP) {
  return updateRegisterVerified(
      DRV8704_REG_DRIVE,
      DRV8704_DRIVE_TDRIVEN_MASK | DRV8704_DRIVE_TDRIVEP_MASK,
      static_cast<uint16_t>(static_cast<uint16_t>(tDriveN) << DRV8704_DRIVE_TDRIVEN_SHIFT) |
          static_cast<uint16_t>(static_cast<uint16_t>(tDriveP) << DRV8704_DRIVE_TDRIVEP_SHIFT),
      DRV8704_DRIVE_TDRIVEN_MASK | DRV8704_DRIVE_TDRIVEP_MASK);
}

bool DRV8704::setGateDriveCurrents(GateDriveSinkCurrent iDriveN, GateDriveSourceCurrent iDriveP) {
  return updateRegisterVerified(
      DRV8704_REG_DRIVE,
      DRV8704_DRIVE_IDRIVEN_MASK | DRV8704_DRIVE_IDRIVEP_MASK,
      static_cast<uint16_t>(static_cast<uint16_t>(iDriveN) << DRV8704_DRIVE_IDRIVEN_SHIFT) |
          static_cast<uint16_t>(static_cast<uint16_t>(iDriveP) << DRV8704_DRIVE_IDRIVEP_SHIFT),
      DRV8704_DRIVE_IDRIVEN_MASK | DRV8704_DRIVE_IDRIVEP_MASK);
}

bool DRV8704::setGateDrive(const GateDriveConfig& gateDrive) {
  drv8704_drive_reg_t drive = {};
  drive.all = cachedRegister(DRV8704_REG_DRIVE);
  drive.bit.ocpth = static_cast<uint16_t>(gateDrive.ocpThreshold);
  drive.bit.ocpdeg = static_cast<uint16_t>(gateDrive.ocpDeglitch);
  drive.bit.tdriven = static_cast<uint16_t>(gateDrive.tDriveN);
  drive.bit.tdrivep = static_cast<uint16_t>(gateDrive.tDriveP);
  drive.bit.idriven = static_cast<uint16_t>(gateDrive.iDriveN);
  drive.bit.idrivep = static_cast<uint16_t>(gateDrive.iDriveP);

  return writeRegisterVerified(
      DRV8704_REG_DRIVE,
      drive.all,
      DRV8704_DRIVE_OCPTH_MASK |
          DRV8704_DRIVE_OCPDEG_MASK |
          DRV8704_DRIVE_TDRIVEN_MASK |
          DRV8704_DRIVE_TDRIVEP_MASK |
          DRV8704_DRIVE_IDRIVEN_MASK |
          DRV8704_DRIVE_IDRIVEP_MASK);
}

bool DRV8704::applyConfig(const DRV8704Config& config) {
  drv8704_ctrl_reg_t ctrl = {};
  ctrl.all = DRV8704_CTRL_DEFAULT;
  ctrl.bit.enbl = config.enabled ? 1U : 0U;
  ctrl.bit.isgain = static_cast<uint16_t>(config.senseGain);
  ctrl.bit.dtime = static_cast<uint16_t>(config.deadTime);

  drv8704_torque_reg_t torque = {};
  torque.bit.torque = config.torque;

  drv8704_off_reg_t off = {};
  off.all = DRV8704_OFF_DEFAULT;
  off.bit.toff = config.offTime;
  off.bit.pwmmode = static_cast<uint16_t>(DRV8704_OFF_PWMMODE_PWM >> DRV8704_OFF_PWMMODE_SHIFT);

  drv8704_blank_reg_t blank = {};
  blank.bit.tblank = config.blankTime;

  drv8704_decay_reg_t decay = {};
  decay.bit.tdecay = config.decayTime;
  decay.bit.decmod = static_cast<uint16_t>(config.decayMode);

  drv8704_drive_reg_t drive = {};
  drive.bit.ocpth = static_cast<uint16_t>(config.gateDrive.ocpThreshold);
  drive.bit.ocpdeg = static_cast<uint16_t>(config.gateDrive.ocpDeglitch);
  drive.bit.tdriven = static_cast<uint16_t>(config.gateDrive.tDriveN);
  drive.bit.tdrivep = static_cast<uint16_t>(config.gateDrive.tDriveP);
  drive.bit.idriven = static_cast<uint16_t>(config.gateDrive.iDriveN);
  drive.bit.idrivep = static_cast<uint16_t>(config.gateDrive.iDriveP);

  return writeRegisterVerified(
             DRV8704_REG_CTRL,
             ctrl.all,
             DRV8704_CTRL_ENBL_MASK | DRV8704_CTRL_ISGAIN_MASK | DRV8704_CTRL_DTIME_MASK) &&
         writeRegisterVerified(
             DRV8704_REG_TORQUE,
             torque.all,
             DRV8704_TORQUE_MASK) &&
         writeRegisterVerified(
             DRV8704_REG_OFF,
             off.all,
             DRV8704_OFF_TOFF_MASK | DRV8704_OFF_PWMMODE_MASK) &&
         writeRegisterVerified(
             DRV8704_REG_BLANK,
             blank.all,
             DRV8704_BLANK_TBLANK_MASK) &&
         writeRegisterVerified(
             DRV8704_REG_DECAY,
             decay.all,
             DRV8704_DECAY_TDECAY_MASK | DRV8704_DECAY_DECMOD_MASK) &&
         writeRegisterVerified(
             DRV8704_REG_DRIVE,
             drive.all,
             DRV8704_DRIVE_OCPTH_MASK |
                 DRV8704_DRIVE_OCPDEG_MASK |
                 DRV8704_DRIVE_TDRIVEN_MASK |
                 DRV8704_DRIVE_TDRIVEP_MASK |
                 DRV8704_DRIVE_IDRIVEN_MASK |
                 DRV8704_DRIVE_IDRIVEP_MASK);
}
