/**
 * @file drv8704_types.h
 * @brief Public enums and data structures for the DRV8704 driver.
 */

#ifndef DRV8704_TYPES_H
#define DRV8704_TYPES_H

#include <stdint.h>

/**
 * @brief Supported sense amplifier gain settings.
 */
enum class SenseGain : uint8_t {
  Gain5VperV = 0,
  Gain10VperV,
  Gain20VperV,
  Gain40VperV
};

/**
 * @brief Supported dead-time settings.
 */
enum class DeadTime : uint8_t {
  Ns410 = 0,
  Ns460,
  Ns670,
  Ns880
};

/**
 * @brief Supported current-decay modes.
 */
enum class DecayMode : uint8_t {
  Slow = 0,
  Fast = 2,
  Mixed = 3,
  AutoMixed = 5
};

/**
 * @brief Supported overcurrent thresholds.
 */
enum class OcpThreshold : uint8_t {
  MilliVolts250 = 0,
  MilliVolts500,
  MilliVolts750,
  MilliVolts1000
};

/**
 * @brief Supported OCP deglitch durations.
 */
enum class OcpDeglitch : uint8_t {
  Us1p05 = 0,
  Us2p10,
  Us4p20,
  Us8p40
};

/**
 * @brief Supported gate-drive timing values.
 */
enum class GateDriveTime : uint8_t {
  Ns263 = 0,
  Ns525,
  Ns1050,
  Ns2100
};

/**
 * @brief Supported source current values for the predriver.
 */
enum class GateDriveSourceCurrent : uint8_t {
  MilliAmps50 = 0,
  MilliAmps100,
  MilliAmps150,
  MilliAmps200
};

/**
 * @brief Supported sink current values for the predriver.
 */
enum class GateDriveSinkCurrent : uint8_t {
  MilliAmps100 = 0,
  MilliAmps200,
  MilliAmps300,
  MilliAmps400
};

/**
 * @brief Bridge selector.
 */
enum class BridgeId : uint8_t {
  A = 0,
  B
};

/**
 * @brief Runtime bridge command.
 */
enum class BridgeMode : uint8_t {
  Coast = 0,
  Forward,
  Reverse,
  Brake
};

/**
 * @brief High-level runtime state for one bridge.
 */
enum class BridgeRuntimeState : uint8_t {
  Coast = 0,
  Brake,
  CurrentDrive,
  PwmDriveWithCurrentLimit
};

/**
 * @brief High-level direction selector for current mode and PWM mode.
 */
enum class Direction : uint8_t {
  Forward = 0,
  Reverse
};

/**
 * @brief Supported platform PWM backend families.
 */
enum class PwmBackendType : uint8_t {
  None = 0,
  GeneralAnalogWrite,
  Esp32Ledc,
  TeensyHardware,
  StmHardware,
  Unsupported
};

/**
 * @brief Predefined load presets for current-mode timing and decay behavior.
 */
enum class CurrentModePreset : uint8_t {
  Heater = 0,
  ThermoelectricCooler,
  MotorSmallInductance,
  MotorMediumInductance,
  MotorLargeInductance
};

/**
 * @brief Selectable fault bits in the STATUS register.
 */
enum class FaultBit : uint8_t {
  Ots = 0,
  Aocp,
  Bocp,
  Apdf,
  Bpdf,
  Uvlo
};

/**
 * @brief Supported register addresses.
 */
enum class RegisterAddress : uint8_t {
  Ctrl = 0x00,
  Torque = 0x01,
  Off = 0x02,
  Blank = 0x03,
  Decay = 0x04,
  Reserved = 0x05,
  Drive = 0x06,
  Status = 0x07
};

/**
 * @brief Pin assignment bundle for a DRV8704 instance.
 */
struct DRV8704Pins {
  uint8_t csPin;
  int8_t sleepPin;
  int8_t resetPin;
  int8_t faultPin;
  int8_t ain1Pin;
  int8_t ain2Pin;
  int8_t bin1Pin;
  int8_t bin2Pin;

  DRV8704Pins()
      : csPin(0),
        sleepPin(-1),
        resetPin(-1),
        faultPin(-1),
        ain1Pin(-1),
        ain2Pin(-1),
        bin1Pin(-1),
        bin2Pin(-1) {}
};

/**
 * @brief Gate-drive configuration group.
 */
struct GateDriveConfig {
  OcpThreshold ocpThreshold;
  OcpDeglitch ocpDeglitch;
  GateDriveTime tDriveN;
  GateDriveTime tDriveP;
  GateDriveSinkCurrent iDriveN;
  GateDriveSourceCurrent iDriveP;

  GateDriveConfig()
      : ocpThreshold(OcpThreshold::MilliVolts500),
        ocpDeglitch(OcpDeglitch::Us2p10),
        tDriveN(GateDriveTime::Ns1050),
        tDriveP(GateDriveTime::Ns1050),
        iDriveN(GateDriveSinkCurrent::MilliAmps400),
        iDriveP(GateDriveSourceCurrent::MilliAmps200) {}
};

/**
 * @brief High-level DRV8704 configuration snapshot.
 */
struct DRV8704Config {
  bool enabled;
  SenseGain senseGain;
  DeadTime deadTime;
  uint8_t torque;
  uint8_t offTime;
  uint8_t blankTime;
  DecayMode decayMode;
  uint8_t decayTime;
  GateDriveConfig gateDrive;

  DRV8704Config()
      : enabled(true),
        senseGain(SenseGain::Gain5VperV),
        deadTime(DeadTime::Ns410),
        torque(0xFF),
        offTime(0x30),
        blankTime(0x80),
        decayMode(DecayMode::Slow),
        decayTime(0x10) {}
};

/**
 * @brief Timing and decay recommendation associated with a current-mode preset.
 */
struct DRV8704CurrentPresetConfig {
  CurrentModePreset preset;
  uint8_t offTime;
  uint8_t blankTime;
  DecayMode decayMode;
  uint8_t decayTime;
  DeadTime deadTime;

  DRV8704CurrentPresetConfig()
      : preset(CurrentModePreset::MotorMediumInductance),
        offTime(0x30),
        blankTime(0x80),
        decayMode(DecayMode::Mixed),
        decayTime(0x10),
        deadTime(DeadTime::Ns460) {}
};

/**
 * @brief Reported direction and runtime mode of one bridge.
 */
struct DRV8704BridgeState {
  BridgeId bridge;
  Direction direction;
  BridgeRuntimeState runtimeState;
  float speedPercent;

  DRV8704BridgeState()
      : bridge(BridgeId::A),
        direction(Direction::Forward),
        runtimeState(BridgeRuntimeState::Coast),
        speedPercent(0.0f) {}
};

/**
 * @brief Derived chip-global current-limit settings and per-bridge consequences.
 */
struct DRV8704CurrentLimitResult {
  bool enabled;
  bool valid;
  float requestedCurrentLimitAmps;
  float referenceShuntOhms;
  float shuntOhmsA;
  float shuntOhmsB;
  SenseGain selectedGain;
  uint8_t torqueDac;
  float ampsPerTorqueCount;
  float achievableMaxCurrentAmps;
  float appliedCurrentLimitAmpsA;
  float appliedCurrentLimitAmpsB;
  CurrentModePreset preset;
  uint32_t pwmFrequencyHz;

  DRV8704CurrentLimitResult()
      : enabled(false),
        valid(false),
        requestedCurrentLimitAmps(0.0f),
        referenceShuntOhms(0.0f),
        shuntOhmsA(0.0f),
        shuntOhmsB(0.0f),
        selectedGain(SenseGain::Gain5VperV),
        torqueDac(0U),
        ampsPerTorqueCount(0.0f),
        achievableMaxCurrentAmps(0.0f),
        appliedCurrentLimitAmpsA(0.0f),
        appliedCurrentLimitAmpsB(0.0f),
        preset(CurrentModePreset::MotorMediumInductance),
        pwmFrequencyHz(0UL) {}
};

/**
 * @brief Requested PWM generator settings.
 */
struct DRV8704PwmConfig {
  uint32_t frequencyHz;
  uint8_t preferredResolutionBits;

  DRV8704PwmConfig()
      : frequencyHz(20000UL),
        preferredResolutionBits(8U) {}
};

/**
 * @brief Resolved platform PWM capabilities for the active backend.
 */
struct DRV8704PwmCapability {
  bool available;
  PwmBackendType backendType;
  uint32_t requestedFrequencyHz;
  uint32_t achievedFrequencyHz;
  uint8_t resolutionBits;
  uint32_t dutySteps;
  float smallestDutyIncrementPercent;

  DRV8704PwmCapability()
      : available(false),
        backendType(PwmBackendType::None),
        requestedFrequencyHz(0UL),
        achievedFrequencyHz(0UL),
        resolutionBits(0U),
        dutySteps(0UL),
        smallestDutyIncrementPercent(0.0f) {}
};

/**
 * @brief Decoded device status and fault state.
 */
struct DRV8704Status {
  uint16_t raw;
  bool overTemperature;
  bool overCurrentA;
  bool overCurrentB;
  bool predriverFaultA;
  bool predriverFaultB;
  bool undervoltage;

  DRV8704Status()
      : raw(0),
        overTemperature(false),
        overCurrentA(false),
        overCurrentB(false),
        predriverFaultA(false),
        predriverFaultB(false),
        undervoltage(false) {}
};

/**
 * @brief Minimal transport/bring-up health result.
 */
struct DRV8704HealthCheck {
  bool spiOk;
  bool initialized;
  bool faultPinActive;
  bool defaultsMatch;
  bool writeReadbackOk;
  uint8_t defaultMatches;
  uint16_t statusRegister;
  bool faultPresent;

  DRV8704HealthCheck()
      : spiOk(false),
        initialized(false),
        faultPinActive(false),
        defaultsMatch(false),
        writeReadbackOk(false),
        defaultMatches(0),
        statusRegister(0),
        faultPresent(false) {}
};

#endif // DRV8704_TYPES_H
