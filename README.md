# UUtzinger_DRV8704

Arduino library for the Texas Instruments DRV8704 dual H-bridge gate driver.

Generated API docs are available at: https://uutzinger.github.io/Arduino_DRV8704/

## Purpose

This library exposes the DRV8704 through one consistent control model:

- set bridge direction explicitly
- choose a bridge runtime state:
  - `Coast`
  - `Brake`
  - `CurrentDrive`
  - `PwmDriveWithCurrentLimit`
- configure current limit once at the chip level
- use presets to choose reasonable decay and timing behavior

It also provides:

- SPI register access
- fault and status helpers
- platform PWM backends

## Hardware Model

The DRV8704 is not a pure SPI motor controller.

It has two separate control surfaces:

- SPI:
  - configuration
  - diagnostics
  - fault/status
  - current-limit related registers
- bridge inputs `AIN1/AIN2/BIN1/BIN2`:
  - forward/reverse/brake/coast behavior
  - external or library-generated PWM

Important hardware constraint:

- `ISGAIN` and `TORQUE` are chip-global
- therefore the high-level current-limit API is chip-global too

## Required Wiring

Minimum useful connections:

- `SCS` to a GPIO used as chip select
- `SCLK`, `SDATI`, `SDATO` to the MCU SPI bus
- `AIN1`, `AIN2`, `BIN1`, `BIN2` to GPIO or PWM-capable pins

Recommended optional connections:

- `SLEEPn`
- `RESET`
- `FAULTn` with pull-up

Board-level requirements still come from the DRV8704 datasheet:

- external N-channel MOSFET H-bridges
- current shunts at the ISEN inputs
- proper VM/VCP/VINT/V5 bypassing
- pull-ups for `FAULTn` and `SDATO`

## Quick Start

```cpp
#include <drv8704.h>

DRV8704Pins pins;
DRV8704* driver = nullptr;

void setup() {
  pins.csPin = 10;
  pins.sleepPin = 9;
  pins.resetPin = 8;
  pins.faultPin = 7;
  pins.ain1Pin = 5;
  pins.ain2Pin = 6;
  pins.bin1Pin = 3;
  pins.bin2Pin = 4;

  static DRV8704 drv(pins);
  driver = &drv;

  if (!driver->begin()) {
    return;
  }
}

void loop() {
}
```

## Control Model

### Direction

Direction is separate from magnitude control:

- `setDirection(BridgeId bridge, Direction direction)`

This direction is then used by both:

- `setCurrent(...)`
- `setSpeed(...)`

### Static Bridge States

Use explicit static bridge commands when you want an immediate override:

- `coast(BridgeId bridge)`
- `brake(BridgeId bridge)`

These states override active current-drive or PWM-drive behavior on that bridge immediately.

### Current Drive

Use current drive when you want the DRV8704 internal current chopper to regulate a bridge in the currently selected direction.

Current drive is intended for inductive loads. Do not present it as a primary control method for resistive heaters.

Primary API:

- `setShuntResistance(float ohms)` depends on your hardware design
- `setShuntResistance(BridgeId bridge, float ohms)`
- `setCurrentModePreset(CurrentModePreset preset)` timing baseline for heater, TEC, or motor loads
- `setCurrentLimit(float amps)`
- `setCurrent(BridgeId bridge, float amps)`
- `disableCurrentLimit()`
- `currentLimitResult()`

Control flow:

1. set shunt value
2. choose preset
3. set direction
4. call `setCurrent(...)`

Current-limit equation used by the driver:

```text
ICHOP = 2.75 * TORQUE / (256 * ISGAIN * RISENSE)
```

The implementation chooses the highest usable gain first because that gives the finest current resolution for a given shunt.

Important limitation:

- the DRV8704 shunt path is used for internal current chopping and comparator behavior
- the chip does not provide direct current telemetry over SPI
- `currentLimitResult()` reports the programmed current-trip settings, not measured load current
- on mostly resistive heaters, bench results may not track requested current linearly even after timing optimization
- for heaters, use PWM mode for control and treat current limit as protection

### PWM With Current Limit

Use PWM mode when you want open-loop duty control while keeping the DRV8704 current limit active during PWM on-time. This is the recommended control path for resistive heaters.

Primary API:

- `beginPwmMode(const DRV8704PwmConfig& config)`
- `setPwmFrequency(uint32_t hz)`
- `setCurrentLimit(float amps)`
- `setDirection(BridgeId bridge, Direction direction)`
- `setSpeed(BridgeId bridge, float percent)`
- `pwmCapability()`

Behavior:

- `setSpeed(..., 0)`: behaves like `coast(...)`
- `setSpeed(..., 100)`: constant-on directional input, no PWM toggling
- `0 < speed < 100`: one bridge input is PWM, the complementary input is held low

High-level PWM control is intentionally framed as PWM with current limit. If you want effectively unbounded drive, deliberately program a very high current limit instead of using a separate unsafe mode.

Default library PWM settings now start from:

- `5 kHz`
- `10-bit` preferred resolution

The interactive examples follow that model by default:

- `pwm ...` commands apply PWM with the stored current limit active
- `full ...` is the explicit no-current-limit comparison mode
- `DRV8704_Test` is the main engineering console for bring-up and bench tuning

## Presets

The presets are starting points for current-limit timing, not guaranteed final answers.

- `Heater`
  - resistive heater-oriented timing baseline
  - bench-tuned default uses `TOFF=0x40`, `TBLANK=0x00`, `AutoMixed` decay on the reference heater-pad setup
  - use this preset with PWM mode for heater control
- `ThermoelectricCooler`
  - mixed-decay starting point for weakly inductive bipolar loads
- `MotorSmallInductance`
  - shorter timing, more aggressive correction
- `MotorMediumInductance`
  - general motor default
- `MotorLargeInductance`
  - longer timing for slower current change

The implementation also adjusts preset timing when PWM mode is active at higher frequency. Validate the preset on real hardware and retune as needed.

## Mode Transitions

The bridge follows the most recent command on that bridge.

Examples:

- current drive to coast:
  - `coast(bridge)`
- current drive to brake:
  - `brake(bridge)`
- current drive to PWM with current limit:
  - `beginPwmMode(...)`
  - `setCurrentLimit(...)`
  - `setDirection(...)`
  - `setSpeed(...)`
- PWM with current limit to current drive:
  - `setDirection(...)`
  - `setCurrent(...)`

The current-limit configuration remains programmed until you change it or call `disableCurrentLimit()`.

## Examples

The example set is organized by workflow:

- `BasicBringup`
- `RegisterHealthCheck`
- `DRV8704_Test`
- `BridgeControlDemo`
- `CurrentModeDemo`
- `CurrentPresetDemo`
- `Current_Test`
- `PwmModeDemo`
- `PWM_Test`

## Example Snippets

### Current Drive

```cpp
driver.setShuntResistance(0.0005f);
driver.setCurrentModePreset(CurrentModePreset::MotorMediumInductance);
driver.setDirection(BridgeId::A, Direction::Forward);
driver.setCurrent(BridgeId::A, 4.0f);
```

### Interactive Engineering Test

Use `examples/DRV8704_Test` when you need one serial console for:

- bring-up checks after `BasicBringup` and `RegisterHealthCheck`
- current-mode testing and timing sweeps
- PWM duty, frequency, and resolution testing
- comparing current-limited drive, PWM drive, and `full` drive on hardware

### PWM With Current Limit

```cpp
DRV8704PwmConfig pwm;
pwm.frequencyHz = 5000;
pwm.preferredResolutionBits = 10;

driver.setCurrentLimit(4.0f);
driver.beginPwmMode(pwm);
driver.setDirection(BridgeId::A, Direction::Forward);
driver.setSpeed(BridgeId::A, 35.0f);
```

### Coast And Brake

```cpp
driver.coast(BridgeId::A);
driver.brake(BridgeId::A);
```

## PWM Backends

Supported PWM backends:

- generic Arduino fallback using `analogWrite()`
- ESP32 LEDC backend
- Teensy hardware PWM backend
- STM hardware PWM backend

The generic Arduino backend does not control PWM frequency directly. Hardware-specific backends report achieved frequency and duty resolution through `pwmCapability()`.

## Faults And Diagnostics

Fault handling is available through:

- the `STATUS` register
- the optional hardware `FAULTn` pin
- `healthCheck()` for startup plausibility and fault reporting

Useful low-level helpers:

- `readRegister()`
- `writeRegister()`
- `updateRegister()`
- `applyConfig()`
- `readStatus()`
- `hasFault()`
- `clearFaults()`

## Limitations

- current-limit settings are chip-global, not per bridge
- if bridge A and bridge B use different shunts, the same chip-global current-limit setting produces different actual limits on the two bridges
- PWM mode is open-loop duty control, not closed-loop speed control
- the DRV8704 does not report measured current over SPI
- preset tuning is only an initial heuristic and still needs hardware validation

## Documentation

- `README.md` provides the top-level usage model
- `docs/` contains generated Doxygen output when documentation has been built
