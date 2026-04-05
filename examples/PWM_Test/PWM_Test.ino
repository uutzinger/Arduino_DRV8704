#include <drv8704.h>
#include <logger.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

namespace {

constexpr BridgeId kBridge = BridgeId::A;
constexpr float kDefaultCurrentLimitAmps = 5.0f;
constexpr float kDefaultShuntOhms = 0.0005f;
constexpr float kDefaultDutyPercent = 25.0f;
constexpr uint32_t kDefaultPwmFrequencyHz = DRV8704_DEFAULT_INPUT_PWM_HZ;
constexpr uint8_t kDefaultResolutionBits = DRV8704_DEFAULT_PWM_RES_BITS;
constexpr size_t kCommandBufferSize = 80;

enum class DriveMode : uint8_t {
  PwmLimited = 0,
  FullBridge
};

char commandBuffer[kCommandBufferSize];
size_t commandLength = 0;
float requestedCurrentLimitAmps = kDefaultCurrentLimitAmps;
float configuredShuntOhms = kDefaultShuntOhms;
float requestedDutyPercent = kDefaultDutyPercent;
uint32_t requestedPwmFrequencyHz = kDefaultPwmFrequencyHz;
uint8_t requestedResolutionBits = kDefaultResolutionBits;
Direction requestedDirection = Direction::Forward;
CurrentModePreset currentPreset = CurrentModePreset::Heater;
DriveMode driveMode = DriveMode::PwmLimited;
bool driveEnabled = false;
bool driverReady = false;

bool applyDriveSettings();

DRV8704Pins makePins() {
  DRV8704Pins pins;
  pins.csPin = 5;
  pins.sleepPin = 11;
  pins.resetPin = A0;
  pins.faultPin = A1;
  pins.ain1Pin = A2;
  pins.ain2Pin = A3;
  pins.bin1Pin = A4;
  pins.bin2Pin = A5;
  return pins;
}

DRV8704& motorDriver() {
  static DRV8704Pins pins = makePins();
  static DRV8704 driver(pins);
  return driver;
}

const char* directionName(Direction direction) {
  return direction == Direction::Forward ? "forward" : "reverse";
}

const char* runtimeStateName(BridgeRuntimeState runtimeState) {
  switch (runtimeState) {
    case BridgeRuntimeState::Coast:
      return "coast";
    case BridgeRuntimeState::Brake:
      return "brake";
    case BridgeRuntimeState::CurrentDrive:
      return "current-drive";
    case BridgeRuntimeState::PwmDriveWithCurrentLimit:
      return "pwm";
  }

  return "unknown";
}

const char* senseGainName(SenseGain gain) {
  switch (gain) {
    case SenseGain::Gain5VperV:
      return "5 V/V";
    case SenseGain::Gain10VperV:
      return "10 V/V";
    case SenseGain::Gain20VperV:
      return "20 V/V";
    case SenseGain::Gain40VperV:
      return "40 V/V";
  }

  return "unknown";
}

const char* driveModeName(DriveMode mode) {
  return mode == DriveMode::PwmLimited ? "pwm-limited" : "full-bridge";
}

const char* presetName(CurrentModePreset preset) {
  switch (preset) {
    case CurrentModePreset::Heater:
      return "heater";
    case CurrentModePreset::ThermoelectricCooler:
      return "tec";
    case CurrentModePreset::MotorSmallInductance:
      return "motor-small";
    case CurrentModePreset::MotorMediumInductance:
      return "motor-medium";
    case CurrentModePreset::MotorLargeInductance:
      return "motor-large";
  }

  return "unknown";
}

const char* pwmBackendName(PwmBackendType backendType) {
  switch (backendType) {
    case PwmBackendType::None:
      return "none";
    case PwmBackendType::GeneralAnalogWrite:
      return "analogWrite";
    case PwmBackendType::Esp32Ledc:
      return "esp32-ledc";
    case PwmBackendType::TeensyHardware:
      return "teensy-hw";
    case PwmBackendType::StmHardware:
      return "stm-hw";
    case PwmBackendType::Unsupported:
      return "unsupported";
  }

  return "unknown";
}

void printCurrentLimit(Stream& out, const DRV8704CurrentLimitResult& result) {
  out.print("Requested current limit (A): ");
  out.println(result.requestedCurrentLimitAmps, 4);
  out.print("Effective current limit on bridge A (A): ");
  out.println(result.appliedCurrentLimitAmpsA, 4);
  out.print("Effective current limit on bridge B (A): ");
  out.println(result.appliedCurrentLimitAmpsB, 4);
  out.print("Reference shunt (ohms): ");
  out.println(result.referenceShuntOhms, 6);
  out.print("Selected gain: ");
  out.println(senseGainName(result.selectedGain));
  out.print("Torque DAC: ");
  out.println(result.torqueDac);
  out.print("Smallest current step (A/count): ");
  out.println(result.ampsPerTorqueCount, 6);
}

void printPwmCapability(Stream& out, const DRV8704PwmCapability& capability) {
  out.print("PWM backend: ");
  out.println(pwmBackendName(capability.backendType));
  out.print("Requested frequency (Hz): ");
  out.println(capability.requestedFrequencyHz);
  out.print("Achieved frequency (Hz): ");
  out.println(capability.achievedFrequencyHz);
  out.print("Resolution bits: ");
  out.println(capability.resolutionBits);
  out.print("Duty steps: ");
  out.println(capability.dutySteps);
  out.print("Smallest increment (%): ");
  out.println(capability.smallestDutyIncrementPercent, 6);
}

void printHelp() {
  Serial.println();
  Serial.println("Commands:");
  Serial.println("  help               - show this help");
  Serial.println("  status             - print drive, PWM, and current-limit state");
  Serial.println("  mode <pwm|full>    - select PWM-with-current-limit or full-bridge mode");
  Serial.println("  preset <heater|tec|ms|mm|ml> - set current-limit preset");
  Serial.println("  limit <amps>       - set PWM current limit in amps");
  Serial.println("  duty <0..100>      - set PWM duty percent");
  Serial.println("  direction <f|r>    - set direction to forward or reverse");
  Serial.println("  freq <hz>          - set PWM frequency");
  Serial.println("  res <bits>         - reinitialize PWM mode with a new resolution");
  Serial.println("  pwm                - apply stored PWM settings");
  Serial.println("  pwm <duty>         - set duty, then apply PWM");
  Serial.println("  pwm <duty> <f|r>   - set duty and direction, then apply PWM");
  Serial.println("  full               - energize bridge A with no current limit");
  Serial.println("  full <f|r>         - full-bridge drive in selected direction");
  Serial.println("  coast              - disable bridge drive");
  Serial.println("  brake              - brake bridge A");
  Serial.println("  clearfaults        - clear DRV8704 latched faults");
  Serial.println();
}

bool parseDirectionToken(const char* token, Direction& direction) {
  if (token == nullptr) {
    return false;
  }

  if (strcmp(token, "f") == 0 || strcmp(token, "forward") == 0) {
    direction = Direction::Forward;
    return true;
  }

  if (strcmp(token, "r") == 0 || strcmp(token, "reverse") == 0) {
    direction = Direction::Reverse;
    return true;
  }

  return false;
}

bool parsePositiveFloat(const char* token, float& value) {
  if (token == nullptr || *token == '\0') {
    return false;
  }

  char* end = nullptr;
  const float parsed = strtof(token, &end);
  if (end == token || *end != '\0' || parsed <= 0.0f) {
    return false;
  }

  value = parsed;
  return true;
}

bool parseDutyPercent(const char* token, float& value) {
  if (token == nullptr || *token == '\0') {
    return false;
  }

  char* end = nullptr;
  const float parsed = strtof(token, &end);
  if (end == token || *end != '\0' || parsed < 0.0f || parsed > 100.0f) {
    return false;
  }

  value = parsed;
  return true;
}

bool parsePositiveUint32(const char* token, uint32_t& value) {
  if (token == nullptr || *token == '\0') {
    return false;
  }

  char* end = nullptr;
  const unsigned long parsed = strtoul(token, &end, 0);
  if (end == token || *end != '\0' || parsed == 0UL) {
    return false;
  }

  value = static_cast<uint32_t>(parsed);
  return true;
}

bool parseResolutionBits(const char* token, uint8_t& value) {
  uint32_t parsed = 0UL;
  if (!parsePositiveUint32(token, parsed) || parsed > 16UL) {
    return false;
  }

  value = static_cast<uint8_t>(parsed);
  return true;
}

bool parsePresetToken(const char* token, CurrentModePreset& preset) {
  if (token == nullptr) {
    return false;
  }

  if (strcmp(token, "heater") == 0) {
    preset = CurrentModePreset::Heater;
    return true;
  }

  if (strcmp(token, "tec") == 0 || strcmp(token, "thermoelectric") == 0) {
    preset = CurrentModePreset::ThermoelectricCooler;
    return true;
  }

  if (strcmp(token, "ms") == 0 || strcmp(token, "motor-small") == 0) {
    preset = CurrentModePreset::MotorSmallInductance;
    return true;
  }

  if (strcmp(token, "mm") == 0 || strcmp(token, "motor-medium") == 0) {
    preset = CurrentModePreset::MotorMediumInductance;
    return true;
  }

  if (strcmp(token, "ml") == 0 || strcmp(token, "motor-large") == 0) {
    preset = CurrentModePreset::MotorLargeInductance;
    return true;
  }

  return false;
}

bool beginOrRebeginPwmMode() {
  DRV8704& driver = motorDriver();
  driver.endPwmMode();

  DRV8704PwmConfig pwmConfig;
  pwmConfig.frequencyHz = requestedPwmFrequencyHz;
  pwmConfig.preferredResolutionBits = requestedResolutionBits;

  if (!driver.beginPwmMode(pwmConfig)) {
    Serial.println("PWM mode unavailable on this target");
    return false;
  }

  return true;
}

void printStatus() {
  DRV8704& driver = motorDriver();
  const DRV8704BridgeState bridgeState = driver.bridgeState(kBridge);
  const DRV8704Status status = driver.readStatus();

  Serial.println();
  Serial.print("Stored current limit (A): ");
  Serial.println(requestedCurrentLimitAmps, 4);
  Serial.print("Configured shunt (ohms): ");
  Serial.println(configuredShuntOhms, 6);
  Serial.print("Current preset: ");
  Serial.println(presetName(currentPreset));
  Serial.print("Stored duty (%): ");
  Serial.println(requestedDutyPercent, 2);
  Serial.print("Stored PWM frequency (Hz): ");
  Serial.println(requestedPwmFrequencyHz);
  Serial.print("Stored resolution bits: ");
  Serial.println(requestedResolutionBits);
  Serial.print("Drive mode: ");
  Serial.println(driveModeName(driveMode));
  Serial.print("Stored direction: ");
  Serial.println(directionName(requestedDirection));
  Serial.print("Drive enabled: ");
  Serial.println(driveEnabled ? "yes" : "no");
  Serial.print("PWM mode enabled: ");
  Serial.println(driver.isPwmModeEnabled() ? "yes" : "no");
  Serial.print("Bridge runtime state: ");
  Serial.println(runtimeStateName(bridgeState.runtimeState));
  Serial.print("Fault pin active: ");
  Serial.println(driver.isFaultPinActive() ? "yes" : "no");
  Serial.print("Fault present: ");
  Serial.println(driver.hasFault() ? "yes" : "no");
  Serial.print("STATUS raw: 0x");
  Serial.println(status.raw, HEX);
  Serial.print("STATUS flags: OT=");
  Serial.print(status.overTemperature ? "1" : "0");
  Serial.print(" AOCP=");
  Serial.print(status.overCurrentA ? "1" : "0");
  Serial.print(" BOCP=");
  Serial.print(status.overCurrentB ? "1" : "0");
  Serial.print(" APDF=");
  Serial.print(status.predriverFaultA ? "1" : "0");
  Serial.print(" BPDF=");
  Serial.print(status.predriverFaultB ? "1" : "0");
  Serial.print(" UVLO=");
  Serial.println(status.undervoltage ? "1" : "0");

  if (driver.isCurrentLimitEnabled()) {
    printCurrentLimit(Serial, driver.currentLimitResult());
  } else {
    Serial.println("Current limit disabled");
  }

  if (driver.isPwmModeEnabled()) {
    printPwmCapability(Serial, driver.pwmCapability());
  } else {
    Serial.println("PWM capability unavailable");
  }

  Serial.println();
}

bool applyDriveSettings() {
  DRV8704& driver = motorDriver();

  if (!driver.setDirection(kBridge, requestedDirection)) {
    Serial.println("Failed to apply direction");
    return false;
  }

  if (driveMode == DriveMode::FullBridge) {
    if (!driver.disableCurrentLimit()) {
      Serial.println("Failed to disable current limit");
      return false;
    }

    driver.setBridgeMode(
        kBridge,
        requestedDirection == Direction::Forward ? BridgeMode::Forward : BridgeMode::Reverse);
    driveEnabled = true;
    Serial.println("Current limit disabled for full-bridge drive");
    return true;
  }

  if (!driver.setCurrentModePreset(currentPreset)) {
    Serial.println("Failed to apply current preset");
    return false;
  }

  if (!driver.setCurrentLimit(requestedCurrentLimitAmps)) {
    Serial.println("Failed to apply current limit");
    return false;
  }

  if (!driver.isPwmModeEnabled() && !beginOrRebeginPwmMode()) {
    return false;
  }

  if (!driver.setSpeed(kBridge, requestedDutyPercent)) {
    Serial.println("Failed to apply PWM duty");
    return false;
  }

  driveEnabled = requestedDutyPercent > 0.0f;
  printCurrentLimit(Serial, driver.currentLimitResult());
  printPwmCapability(Serial, driver.pwmCapability());
  return true;
}

void handleModeCommand(const char* token) {
  if (token == nullptr) {
    Serial.println("Usage: mode <pwm|full>");
    return;
  }

  if (strcmp(token, "pwm") == 0 || strcmp(token, "limited") == 0) {
    driveMode = DriveMode::PwmLimited;
  } else if (strcmp(token, "full") == 0 || strcmp(token, "fullbridge") == 0) {
    driveMode = DriveMode::FullBridge;
  } else {
    Serial.println("Usage: mode <pwm|full>");
    return;
  }

  Serial.print("Drive mode set to ");
  Serial.println(driveModeName(driveMode));

  if (driveEnabled) {
    applyDriveSettings();
  }
}

void handlePresetCommand(const char* token) {
  CurrentModePreset preset;
  if (!parsePresetToken(token, preset)) {
    Serial.println("Usage: preset <heater|tec|ms|mm|ml>");
    return;
  }

  currentPreset = preset;
  Serial.print("Current preset set to ");
  Serial.println(presetName(currentPreset));

  if (driveEnabled && driveMode == DriveMode::PwmLimited) {
    applyDriveSettings();
  }
}

void handleLimitCommand(const char* token) {
  float amps = 0.0f;
  if (!parsePositiveFloat(token, amps)) {
    Serial.println("Usage: limit <amps>, for example: limit 4.0");
    return;
  }

  requestedCurrentLimitAmps = amps;
  Serial.print("Stored current limit updated to ");
  Serial.print(requestedCurrentLimitAmps, 4);
  Serial.println(" A");

  if (driveEnabled && driveMode == DriveMode::PwmLimited) {
    applyDriveSettings();
  }
}

void handleDutyCommand(const char* token) {
  float duty = 0.0f;
  if (!parseDutyPercent(token, duty)) {
    Serial.println("Usage: duty <0..100>");
    return;
  }

  requestedDutyPercent = duty;
  Serial.print("Stored duty updated to ");
  Serial.print(requestedDutyPercent, 2);
  Serial.println(" %");

  if (driveEnabled && driveMode == DriveMode::PwmLimited) {
    applyDriveSettings();
  }
}

void handleDirectionCommand(const char* token) {
  Direction direction;
  if (!parseDirectionToken(token, direction)) {
    Serial.println("Usage: direction <f|r>");
    return;
  }

  requestedDirection = direction;
  Serial.print("Stored direction updated to ");
  Serial.println(directionName(requestedDirection));

  if (driveEnabled) {
    applyDriveSettings();
  } else if (!motorDriver().setDirection(kBridge, requestedDirection)) {
    Serial.println("Failed to store direction in driver");
  }
}

void handleFrequencyCommand(const char* token) {
  uint32_t frequencyHz = 0UL;
  if (!parsePositiveUint32(token, frequencyHz)) {
    Serial.println("Usage: freq <hz>");
    return;
  }

  requestedPwmFrequencyHz = frequencyHz;
  Serial.print("Stored PWM frequency updated to ");
  Serial.print(requestedPwmFrequencyHz);
  Serial.println(" Hz");

  DRV8704& driver = motorDriver();
  if (driver.isPwmModeEnabled()) {
    if (!driver.setPwmFrequency(requestedPwmFrequencyHz)) {
      Serial.println("Failed to set PWM frequency");
      return;
    }
    printPwmCapability(Serial, driver.pwmCapability());
  }

  if (driveEnabled && driveMode == DriveMode::PwmLimited) {
    applyDriveSettings();
  }
}

void handleResolutionCommand(const char* token) {
  uint8_t bits = 0U;
  if (!parseResolutionBits(token, bits)) {
    Serial.println("Usage: res <bits>");
    return;
  }

  requestedResolutionBits = bits;
  Serial.print("Stored PWM resolution updated to ");
  Serial.print(requestedResolutionBits);
  Serial.println(" bits");

  if (!beginOrRebeginPwmMode()) {
    return;
  }

  printPwmCapability(Serial, motorDriver().pwmCapability());
  if (driveEnabled && driveMode == DriveMode::PwmLimited) {
    applyDriveSettings();
  }
}

void handlePwmCommand(char* dutyToken, char* directionToken) {
  float duty = requestedDutyPercent;
  Direction direction = requestedDirection;

  if (dutyToken != nullptr) {
    if (!parseDutyPercent(dutyToken, duty)) {
      if (directionToken == nullptr && parseDirectionToken(dutyToken, direction)) {
        dutyToken = nullptr;
      } else {
        Serial.println("Usage: pwm [duty] [f|r]");
        return;
      }
    }
  }

  if (directionToken != nullptr && !parseDirectionToken(directionToken, direction)) {
    Serial.println("Usage: pwm [duty] [f|r]");
    return;
  }

  requestedDutyPercent = duty;
  requestedDirection = direction;
  driveMode = DriveMode::PwmLimited;

  if (applyDriveSettings()) {
    Serial.print("PWM drive ");
    Serial.print(requestedDutyPercent, 2);
    Serial.print(" % ");
    Serial.println(directionName(requestedDirection));
  }
}

void handleFullCommand(char* directionToken) {
  Direction direction = requestedDirection;
  if (directionToken != nullptr && !parseDirectionToken(directionToken, direction)) {
    Serial.println("Usage: full [f|r]");
    return;
  }

  requestedDirection = direction;
  driveMode = DriveMode::FullBridge;

  if (applyDriveSettings()) {
    Serial.print("Driving full-bridge ");
    Serial.println(directionName(requestedDirection));
  }
}

void handleShuntCommand(const char* token) {
  float ohms = 0.0f;
  if (!parsePositiveFloat(token, ohms)) {
    Serial.println("Usage: shunt <ohms>, for example: shunt 0.005");
    return;
  }

  if (!motorDriver().setShuntResistance(ohms)) {
    Serial.println("Failed to apply shunt resistance");
    return;
  }

  configuredShuntOhms = ohms;
  Serial.print("Configured shunt updated to ");
  Serial.print(configuredShuntOhms, 6);
  Serial.println(" ohm");

  if (driveEnabled) {
    applyDriveSettings();
  }
}

void handleCommand(char* line) {
  for (char* p = line; *p != '\0'; ++p) {
    *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
  }

  char* command = strtok(line, " \t");
  if (command == nullptr) {
    return;
  }

  if (strcmp(command, "help") == 0 || strcmp(command, "?") == 0) {
    printHelp();
    return;
  }

  if (strcmp(command, "status") == 0) {
    printStatus();
    return;
  }

  if (strcmp(command, "mode") == 0) {
    handleModeCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "preset") == 0) {
    handlePresetCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "limit") == 0) {
    handleLimitCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "duty") == 0) {
    handleDutyCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "direction") == 0 || strcmp(command, "dir") == 0) {
    handleDirectionCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "freq") == 0) {
    handleFrequencyCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "res") == 0) {
    handleResolutionCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "pwm") == 0 || strcmp(command, "drive") == 0 || strcmp(command, "on") == 0) {
    handlePwmCommand(strtok(nullptr, " \t"), strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "full") == 0) {
    handleFullCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "shunt") == 0) {
    handleShuntCommand(strtok(nullptr, " \t"));
    return;
  }

  if (strcmp(command, "coast") == 0 || strcmp(command, "off") == 0 || strcmp(command, "stop") == 0) {
    driveEnabled = false;
    motorDriver().coast(kBridge);
    Serial.println("Bridge A set to coast");
    return;
  }

  if (strcmp(command, "brake") == 0) {
    driveEnabled = false;
    motorDriver().brake(kBridge);
    Serial.println("Bridge A brake applied");
    return;
  }

  if (strcmp(command, "clearfaults") == 0) {
    if (motorDriver().clearFaults()) {
      Serial.println("Faults cleared");
    } else {
      Serial.println("Failed to clear faults");
    }
    return;
  }

  Serial.println("Unknown command. Type 'help' for a command list.");
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      commandBuffer[commandLength] = '\0';
      handleCommand(commandBuffer);
      commandLength = 0;
      continue;
    }

    if (commandLength + 1U < kCommandBufferSize) {
      commandBuffer[commandLength++] = incoming;
    } else {
      commandLength = 0;
      Serial.println("Command too long");
    }
  }
}

}  // namespace

void setup() {
  pinMode(6, OUTPUT);
  digitalWrite(6, HIGH);
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);

  Serial.begin(115200);
  delay(1000);
  currentLogLevel = LOG_LEVEL_INFO;

  DRV8704& driver = motorDriver();
  if (!driver.begin()) {
    Serial.println("DRV8704 begin failed");
    return;
  }

  if (!driver.setShuntResistance(configuredShuntOhms)) {
    Serial.println("Invalid shunt resistance");
    return;
  }

  if (!driver.setCurrentModePreset(currentPreset)) {
    Serial.println("Failed to apply default current preset");
    return;
  }

  if (!driver.setCurrentLimit(requestedCurrentLimitAmps)) {
    Serial.println("Failed to apply default current limit");
    return;
  }

  if (!beginOrRebeginPwmMode()) {
    return;
  }

  if (!driver.setDirection(kBridge, requestedDirection)) {
    Serial.println("Failed to apply initial direction");
    return;
  }

  driver.coast(kBridge);
  driverReady = true;

  Serial.println();
  Serial.println("DRV8704 PWM test ready");
  Serial.println("Bridge A starts in coast for safety");
  printHelp();
  printStatus();
}

void loop() {
  if (!driverReady) {
    return;
  }

  readSerialCommands();
}
