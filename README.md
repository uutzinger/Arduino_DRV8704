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

Primary API:

- `setShuntResistance(float ohms)` depends on your hardware design
- `setShuntResistance(BridgeId bridge, float ohms)`
- `setCurrentModePreset(CurrentModePreset preset)` default settings for heater, tec or motors
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

### PWM With Current Limit

Use PWM mode when you want open-loop duty control while keeping the DRV8704 current limit active during PWM on-time.

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

## Presets

The current-limit presets are practical starting points:

- `Heater`
  - resistive load
  - slower, quieter decay behavior
- `ThermoelectricCooler`
  - weakly inductive bipolar load
  - mixed-decay middle ground
- `MotorSmallInductance`
  - more aggressive correction
  - shorter timing, faster decay
- `MotorMediumInductance`
  - default motor preset
- `MotorLargeInductance`
  - slower current change
  - longer timing, auto mixed decay

The implementation also adjusts preset timing when PWM mode is active at higher frequency. That is only a starting heuristic, not a substitute for bench tuning.

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
- `BridgeControlDemo`
- `CurrentModeDemo`
- `CurrentPresetDemo`
- `PwmModeDemo`

## Example Snippets

### Current Drive

```cpp
driver.setShuntResistance(0.0005f);
driver.setCurrentModePreset(CurrentModePreset::MotorMediumInductance);
driver.setDirection(BridgeId::A, Direction::Forward);
driver.setCurrent(BridgeId::A, 4.0f);
```

### PWM With Current Limit

```cpp
DRV8704PwmConfig pwm;
pwm.frequencyHz = 20000;
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
