/**
 * @file drv8704.h
 * @brief Public Arduino API for the DRV8704 dual H-bridge gate driver.
 */

#ifndef DRV8704_H
#define DRV8704_H

#include <Arduino.h>
#include <SPI.h>

#include "drv8704_defs.h"
#include "drv8704_types.h"
#include "drv8704_regs_typedefs.h"

class DRV8704PwmBackend;

/**
 * @brief Arduino-facing DRV8704 device driver class.
 */
class DRV8704 {
public:
  /**
   * @brief Construct a DRV8704 device instance.
   * @param pins Pin assignment bundle for this device.
   * @param spi SPI bus instance to use for transfers.
   */
  explicit DRV8704(const DRV8704Pins& pins, SPIClass& spi = SPI);

  /**
   * @brief Initialize GPIO and SPI state for the device.
   * @return True if startup completed without transport failure.
   */
  bool begin();

  /**
   * @brief End driver operation.
   */
  void end();

  /**
   * @brief Put the DRV8704 into sleep mode when the sleep pin is available.
   */
  void sleep();

  /**
   * @brief Bring the DRV8704 out of sleep mode when the sleep pin is available.
   */
  void wake();

  /**
   * @brief Pulse the reset pin when available.
   */
  void reset();

  /**
   * @brief Read a 12-bit register payload from the device.
   * @param address Register address.
   * @return Register payload masked to 12 bits.
   */
  uint16_t readRegister(uint8_t address);

  /**
   * @brief Read a register by symbolic address.
   * @param address Register address enum.
   * @return Register payload masked to 12 bits.
   */
  uint16_t readRegister(RegisterAddress address) {
    return readRegister(static_cast<uint8_t>(address));
  }

  /**
   * @brief Write a 12-bit register payload to the device.
   * @param address Register address.
   * @param value Register payload.
   * @return True if the write transfer completed.
   */
  bool writeRegister(uint8_t address, uint16_t value);

  /**
   * @brief Write a register by symbolic address.
   * @param address Register address enum.
   * @param value Register payload.
   * @return True if the write transfer completed.
   */
  bool writeRegister(RegisterAddress address, uint16_t value) {
    return writeRegister(static_cast<uint8_t>(address), value);
  }

  /**
   * @brief Update selected register bits while preserving other bits.
   * @param address Register address.
   * @param mask Bit mask for fields to update.
   * @param value New masked field value.
   * @return True if the write transfer completed.
   */
  bool updateRegister(uint8_t address, uint16_t mask, uint16_t value);

  /**
   * @brief Update selected register bits using a symbolic address.
   * @param address Register address enum.
   * @param mask Bit mask for fields to update.
   * @param value New masked field value.
   * @return True if the write transfer completed.
   */
  bool updateRegister(RegisterAddress address, uint16_t mask, uint16_t value) {
    return updateRegister(static_cast<uint8_t>(address), mask, value);
  }

  /**
   * @brief Enable or disable the bridge driver through the CTRL register.
   * @param enabled True to enable bridge drive.
   * @return True if the operation completed.
   */
  bool enableBridgeDriver(bool enabled);

  /**
   * @brief Set the current-sense amplifier gain.
   * @param gain Desired sense amplifier gain.
   * @return True if the register update and readback verification completed.
   */
  bool setSenseGain(SenseGain gain);

  /**
   * @brief Set the bridge dead time.
   * @param deadTime Desired dead-time setting.
   * @return True if the register update and readback verification completed.
   */
  bool setDeadTime(DeadTime deadTime);

  /**
   * @brief Set the torque DAC value.
   * @param torque Torque DAC code.
   * @return True if the register update and readback verification completed.
   */
  bool setTorque(uint8_t torque);

  /**
   * @brief Set the fixed PWM off time.
   * @param offTime OFF register TOFF value.
   * @return True if the register update and readback verification completed.
   */
  bool setOffTime(uint8_t offTime);

  /**
   * @brief Set the current blanking time.
   * @param blankTime BLANK register TBLANK value.
   * @return True if the register update and readback verification completed.
   */
  bool setBlankTime(uint8_t blankTime);

  /**
   * @brief Set the decay mode.
   * @param mode Desired decay mode.
   * @return True if the register update and readback verification completed.
   */
  bool setDecayMode(DecayMode mode);

  /**
   * @brief Set the mixed-decay transition time.
   * @param decayTime DECAY register TDECAY value.
   * @return True if the register update and readback verification completed.
   */
  bool setDecayTime(uint8_t decayTime);

  /**
   * @brief Set the overcurrent threshold.
   * @param threshold Desired OCP threshold.
   * @return True if the register update and readback verification completed.
   */
  bool setOcpThreshold(OcpThreshold threshold);

  /**
   * @brief Set the overcurrent deglitch interval.
   * @param deglitch Desired OCP deglitch setting.
   * @return True if the register update and readback verification completed.
   */
  bool setOcpDeglitch(OcpDeglitch deglitch);

  /**
   * @brief Set the gate-drive timing fields.
   * @param tDriveN Sink timing selection.
   * @param tDriveP Source timing selection.
   * @return True if the register update and readback verification completed.
   */
  bool setGateDriveTimes(GateDriveTime tDriveN, GateDriveTime tDriveP);

  /**
   * @brief Set the gate-drive current fields.
   * @param iDriveN Sink current selection.
   * @param iDriveP Source current selection.
   * @return True if the register update and readback verification completed.
   */
  bool setGateDriveCurrents(GateDriveSinkCurrent iDriveN, GateDriveSourceCurrent iDriveP);

  /**
   * @brief Apply the complete DRIVE register configuration.
   * @param gateDrive Desired gate-drive configuration.
   * @return True if the register write and readback verification completed.
   */
  bool setGateDrive(const GateDriveConfig& gateDrive);

  /**
   * @brief Apply a high-level configuration snapshot to the device.
   * @param config Desired driver configuration.
   * @return True if the configuration sequence completed.
   */
  bool applyConfig(const DRV8704Config& config);

  /**
   * @brief Set one shunt resistance value for both bridges in current mode.
   * @param ohms Sense resistor value in ohms.
   * @return True when the provided value is valid.
   */
  bool setShuntResistance(float ohms);

  /**
   * @brief Set the shunt resistance for one bridge in current mode.
   * @param bridge Bridge selector.
   * @param ohms Sense resistor value in ohms.
   * @return True when the provided value is valid.
   */
  bool setShuntResistance(BridgeId bridge, float ohms);

  /**
   * @brief Select the predefined timing and decay preset used by current mode.
   * @param preset Preset family to apply.
   * @return True when the preset configuration was written successfully.
   */
  bool setCurrentModePreset(CurrentModePreset preset);

  /**
   * @brief Set the direction used by high-level current-drive and PWM-drive commands.
   * @param bridge Bridge selector.
   * @param direction Desired bridge direction.
   * @return True when the bridge direction was accepted.
   */
  bool setDirection(BridgeId bridge, Direction direction);

  /**
   * @brief Return the configured high-level direction for one bridge.
   * @param bridge Bridge selector.
   * @return Stored bridge direction.
   */
  Direction direction(BridgeId bridge) const;

  /**
   * @brief Program the chip-global current limit in amperes without changing bridge state.
   * @param amps Requested current limit in amperes.
   * @return True when the requested limit was derived and programmed successfully.
   */
  bool setCurrentLimit(float amps);

  /**
   * @brief Program the chip for current drive on one bridge using the stored direction.
   * @param bridge Bridge selector.
   * @param amps Requested current limit in amperes.
   * @return True when the current-limit request was valid and the bridge entered current drive.
   */
  bool setCurrent(BridgeId bridge, float amps);

  /**
   * @brief Disable the high-level current-limit configuration without forcing a bridge-state change.
   * @return True when the high-level current-limit state was cleared.
   * @details The implementation programs the maximum representable current limit and clears the
   * high-level bookkeeping so future PWM-current-limit commands require a deliberate limit request.
   */
  bool disableCurrentLimit();

  /**
   * @brief Return whether a high-level current limit is currently configured.
   * @return True when current-limit settings are armed for current-drive or PWM-current-limit use.
   */
  bool isCurrentLimitEnabled() const { return currentLimitEnabled_; }

  /**
   * @brief Return the preset configuration currently selected for current-limit operation.
   * @return Current-limit preset settings.
   */
  DRV8704CurrentPresetConfig currentModePresetConfig() const { return currentPresetConfig_; }

  /**
   * @brief Return the most recent derived chip-global current-limit settings.
   * @return Current-limit result structure.
   */
  DRV8704CurrentLimitResult currentLimitResult() const { return currentLimitResult_; }

  /**
   * @brief Read and decode the STATUS register.
   * @return Decoded status structure.
   */
  DRV8704Status readStatus();

  /**
   * @brief Check whether any reported fault bit is active.
   * @return True if a fault is reported.
   */
  bool hasFault();

  /**
   * @brief Report whether a dedicated `FAULTn` pin was provided.
   * @return True if the device instance can monitor the hardware fault pin.
   */
  bool hasFaultPin() const { return pins_.faultPin >= 0; }

  /**
   * @brief Read the state of the optional `FAULTn` pin.
   * @return True when the open-drain fault pin is asserted low.
   */
  bool isFaultPinActive() const;

  /**
   * @brief Clear all clearable fault bits in the STATUS register.
   * @return True if the clear write completed.
   */
  bool clearFaults();

  /**
   * @brief Clear one fault bit in the STATUS register.
   * @param bit Fault bit selector.
   * @return True if the clear write completed.
   */
  bool clearFault(FaultBit bit);

  /**
   * @brief Drive one bridge to coast.
   * @param bridge Bridge selector.
   */
  void coast(BridgeId bridge);

  /**
   * @brief Drive one bridge to brake.
   * @param bridge Bridge selector.
   */
  void brake(BridgeId bridge);

  /**
   * @brief Low-level static bridge helper retained for compatibility.
   * @param bridge Bridge selector.
   * @param mode Static bridge mode.
   */
  void setBridgeMode(BridgeId bridge, BridgeMode mode);

  /**
   * @brief Return the stored direction/runtime-state report for one bridge.
   * @param bridge Bridge selector.
   * @return Bridge runtime state report.
   */
  DRV8704BridgeState bridgeState(BridgeId bridge) const;

  /**
   * @brief Enable board-specific PWM generation for the DRV8704 input pins.
   * @param config Requested PWM frequency and duty resolution.
   * @return True when a supported PWM backend was initialized.
   */
  bool beginPwmMode(const DRV8704PwmConfig& config = DRV8704PwmConfig());

  /**
   * @brief Disable platform PWM generation and return bridge inputs to coast.
   */
  void endPwmMode();

  /**
   * @brief Update the requested PWM frequency.
   * @param frequencyHz Requested PWM frequency in hertz.
   * @return True when the active backend accepted the new frequency.
   */
  bool setPwmFrequency(uint32_t frequencyHz);

  /**
   * @brief Apply an open-loop speed command to one bridge using hardware PWM.
   * @param bridge Bridge selector.
   * @param speedPercent Open-loop duty request from 0 to 100 percent.
   * @return True when the requested speed was applied.
   * @details This is PWM with current limit. A speed of 0 behaves like coast.
   * A speed of 100 forces a constant-on directional input without PWM toggling.
   * Intermediate values apply hardware PWM using the stored bridge direction.
   */
  bool setSpeed(BridgeId bridge, float speedPercent);

  /**
   * @brief Compatibility overload retained during the control-model refactor.
   * @param bridge Bridge selector.
   * @param direction Desired bridge direction.
   * @param speedPercent Open-loop duty request from 0 to 100 percent.
   * @return True when the requested speed was applied.
   */
  bool setSpeed(BridgeId bridge, Direction direction, float speedPercent) {
    return setDirection(bridge, direction) && setSpeed(bridge, speedPercent);
  }

  /**
   * @brief Compatibility overload retained during the control-model refactor.
   * @param bridge Bridge selector.
   * @param direction Desired bridge direction.
   * @param amps Requested current in amperes.
   * @return True when the request was applied.
   */
  bool setCurrent(BridgeId bridge, Direction direction, float amps) {
    return setDirection(bridge, direction) && setCurrent(bridge, amps);
  }

  /**
   * @brief Return the resolved capability of the active PWM backend.
   * @return Active PWM capability report.
   */
  DRV8704PwmCapability pwmCapability() const { return pwmCapability_; }

  /**
   * @brief Return the smallest duty-percent step of the active PWM backend.
   * @return Smallest achievable duty increment in percent.
   */
  float smallestDutyIncrementPercent() const {
    return pwmCapability_.smallestDutyIncrementPercent;
  }

  /**
   * @brief Return whether PWM mode is currently active.
   * @return True when a supported PWM backend is enabled.
   */
  bool isPwmModeEnabled() const { return pwmModeEnabled_; }

  /**
   * @brief Perform a minimal transport/status health check.
   * @return Health-check result.
   */
  DRV8704HealthCheck healthCheck();

  /**
   * @brief Report whether the driver completed initialization.
   * @return True if begin() completed successfully.
   */
  bool isInitialized() const { return initialized_; }

  /**
   * @brief Return the cached contents of a register.
   * @param address Register address.
   * @return Cached register payload or 0 for an invalid address.
   */
  uint16_t cachedRegister(uint8_t address) const;

  /**
   * @brief Refresh the local register cache from device reads.
   * @return True if all register reads completed.
   */
  bool syncRegisterCache();

private:
  /**
   * @brief Check whether a raw register address is valid for the DRV8704.
   * @param address Raw register address.
   * @return True when the address is within the implemented register range.
   */
  bool isValidAddress(uint8_t address) const;

  /**
   * @brief Write a register and verify the relevant bits by reading it back.
   * @param address Register address.
   * @param value Register payload to write.
   * @param verifyMask Bit mask used to compare readback content.
   * @return True when the masked readback matches the masked write value.
   */
  bool writeRegisterVerified(uint8_t address, uint16_t value, uint16_t verifyMask);

  /**
   * @brief Update selected register bits and verify the resulting readback.
   * @param address Register address.
   * @param mask Mask of bits to modify.
   * @param value New masked field value.
   * @param verifyMask Bit mask used to compare readback content.
   * @return True when the masked readback matches the expected result.
   */
  bool updateRegisterVerified(uint8_t address, uint16_t mask, uint16_t value, uint16_t verifyMask);

  /**
   * @brief Seed the local register cache with datasheet reset defaults.
   */
  void seedDefaultRegisterCache();

  /**
   * @brief Compare key readable registers against datasheet reset defaults.
   * @param matchCount Number of registers that matched their expected reset values.
   * @return True when all key startup registers matched expected defaults.
   */
  bool checkDefaultRegisters(uint8_t& matchCount);

  /**
   * @brief Log one default-register mismatch during startup health checking.
   * @param address Register address being checked.
   * @param expected Expected reset/default value.
   * @param actual Actual value read from the device.
   */
  void logDefaultRegisterMismatch(uint8_t address, uint16_t expected, uint16_t actual) const;

  /**
   * @brief Return the numeric V/V gain corresponding to the enum selection.
   * @param gain Sense-gain enum.
   * @return Gain value in volts per volt.
   */
  float senseGainValue(SenseGain gain) const;

  /**
   * @brief Return the shunt resistance currently assigned to one bridge.
   * @param bridge Bridge selector.
   * @return Shunt resistance in ohms.
   */
  float shuntResistance(BridgeId bridge) const;

  /**
   * @brief Return the shunt resistance used as the safe chip-global current-limit reference.
   * @return Reference shunt resistance in ohms.
   */
  float currentLimitReferenceShunt() const;

  /**
   * @brief Build the recommended config overlay for one current-mode preset.
   * @param preset Preset selector.
   * @param pwmFrequencyHz External PWM frequency in hertz, or 0 for current-drive operation.
   * @return Preset timing and decay recommendation.
   */
  DRV8704CurrentPresetConfig presetConfig(CurrentModePreset preset, uint32_t pwmFrequencyHz) const;

  /**
   * @brief Derive chip-global gain and torque settings for a requested current limit.
   * @param amps Requested current limit in amperes.
   * @param result Derived output structure.
   * @return True when the request is valid and representable by the device.
   */
  bool deriveCurrentLimit(float amps, DRV8704CurrentLimitResult& result) const;

  /**
   * @brief Apply one derived current-limit result to the chip registers.
   * @param result Derived output structure.
   * @param pwmFrequencyHz PWM frequency in hertz, or 0 for current-drive operation.
   * @return True when the current-limit configuration was written successfully.
   */
  bool applyCurrentLimitResult(const DRV8704CurrentLimitResult& result, uint32_t pwmFrequencyHz);

  /**
   * @brief Apply the stored state for one bridge after a direction or mode change.
   * @param bridge Bridge selector.
   * @return True when the bridge outputs were updated successfully.
   */
  bool applyBridgeState(BridgeId bridge);

  /**
   * @brief Verify bidirectional SPI communication with a write/readback probe.
   * @return True when the probe write reads back correctly and the original value is restored.
   */
  bool performWriteReadbackProbe();

  /**
   * @brief Assert the active-high `SCS` signal and begin an SPI transaction.
   */
  void beginTransaction();

  /**
   * @brief Deassert the active-high `SCS` signal and end the SPI transaction.
   */
  void endTransaction();

  /**
   * @brief Transfer one raw 16-bit SPI frame.
   * @param frame Raw DRV8704 SPI frame.
   * @return Raw 16-bit SPI response frame.
   */
  uint16_t transferFrame(uint16_t frame);

  /**
   * @brief Apply one static bridge command to an IN1/IN2 input pair.
   * @param in1Pin First bridge input pin.
   * @param in2Pin Second bridge input pin.
   * @param mode Static bridge mode to apply.
   */
  void setBridgePins(int8_t in1Pin, int8_t in2Pin, BridgeMode mode);

  /**
   * @brief Disable PWM output on one bridge and drive it to a static state.
   * @param bridge Bridge selector.
   * @param mode Static mode to apply after PWM is released.
   */
  void setBridgeStaticMode(BridgeId bridge, BridgeMode mode);

  /**
   * @brief Configure one bridge for a directional PWM command.
   * @param in1Pin First bridge input pin.
   * @param in2Pin Second bridge input pin.
   * @param direction Commanded direction.
   * @param speedPercent Open-loop duty request from 0 to 100 percent.
   * @return True when the command was applied through the PWM backend.
   */
  bool setBridgePwm(int8_t in1Pin, int8_t in2Pin, Direction direction, float speedPercent);

  DRV8704Pins pins_;
  SPIClass* spi_;
  SPISettings spiSettings_;
  uint16_t registerCache_[DRV8704_REGISTER_COUNT];
  float shuntOhms_[2];
  Direction bridgeDirections_[2];
  DRV8704BridgeState bridgeStates_[2];
  float bridgeSpeedPercents_[2];
  CurrentModePreset currentPreset_;
  DRV8704CurrentPresetConfig currentPresetConfig_;
  DRV8704CurrentLimitResult currentLimitResult_;
  bool currentLimitEnabled_;
  DRV8704PwmBackend* pwmBackend_;
  DRV8704PwmConfig pwmConfig_;
  DRV8704PwmCapability pwmCapability_;
  bool pwmModeEnabled_;
  bool initialized_;
};

#endif // DRV8704_H
