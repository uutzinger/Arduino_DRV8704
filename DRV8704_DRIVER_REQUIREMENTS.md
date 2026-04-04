# DRV8704 Driver Requirements and Implementation Plan

## 1. Purpose

This document defines the requirements and development plan for a new Arduino-style driver for the Texas Instruments DRV8704 dual H-bridge PWM gate driver.

The existing implementation in `src/drv.h` and `src/drv.cpp` is treated as a previous attempt and should be replaced rather than extended.

This plan is based on:

- The DRV8704 datasheet in `assets/drv8704.pdf`
- The existing library structure used in `~/Documents/GitHub/BioMedicalSensorBoard/Arduino/libraries/UUtzinger_MAX30001G/src`
- The documentation and script tooling used in `~/Documents/GitHub/BioMedicalSensorBoard/Arduino/libraries/UUtzinger_MAX30001G`

Note: the reference file in `assets` is `drv8704.pdf`, not `drv8740.pfd`.

## 2. Device Summary

The DRV8704 is not a complete motor driver with integrated power FETs. It is a dual brushed-DC motor gate driver for external N-channel MOSFET H-bridges.

The device has two kinds of control surfaces:

- Hardware control pins:
  - `SLEEPn`
  - `RESET`
  - `AIN1`, `AIN2`
  - `BIN1`, `BIN2`
- SPI configuration and status registers:
  - `CTRL`
  - `TORQUE`
  - `OFF`
  - `BLANK`
  - `DECAY`
  - `DRIVE`
  - `STATUS`

This distinction matters for the software architecture:

- Dynamic motor direction and PWM operation happen through GPIO pins, not through SPI register writes.
- SPI is used for configuration, diagnostics, current-regulation tuning, and fault/status handling.
- A higher-level driver may still generate those GPIO PWM signals internally on supported MCUs such as ESP32 and Teensy.

## 3. Design Goals

The new driver should:

- Provide a clean, documented, maintainable replacement for the current `drv` class.
- Follow the modular structure style used by `UUtzinger_MAX30001G`.
- Include matching documentation assets and documentation-generation workflow.
- Be packaged as a standard Arduino library suitable for GitHub distribution and Arduino library registration.
- Expose the DRV8704 capabilities without forcing string-based APIs.
- Use enums, typed configuration structures, and explicit register helpers.
- Support both configuration-time use and runtime motor control support.
- Provide a high-level current-control mode where the user commands direction and current in amperes.
- Provide a hardware PWM mode for supported MCU families where the user commands direction and speed in percent.
- Keep hardware-specific logic contained and easy to test.
- Avoid hidden global state.

## 4. Non-Goals

The first replacement version should not try to solve system-level motor control beyond the DRV8704 itself.

Out of scope for the base driver:

- Closed-loop speed control
- Encoder support
- Motion planning
- Automatic MOSFET selection or board-level electrical validation
- RTOS-specific abstractions
- Generic multi-platform HAL support beyond Arduino SPI/GPIO and explicitly supported PWM backends

## 5. Functional Requirements

## 5.1 Initialization and Lifecycle

The driver shall:

- Construct from pin definitions required by the chip:
  - SPI chip-select `SCS`
  - Optional `SLEEPn`
  - Optional `RESET`
  - Optional `FAULTn`
  - Optional bridge input pins `AIN1`, `AIN2`, `BIN1`, `BIN2`
- Configure pin modes during `begin()`
- Initialize SPI settings compatible with the DRV8704 serial interface
- Support bringing the device out of sleep
- Support hardware reset if a reset pin is provided
- Read and cache initial register contents after startup
- Provide `end()` or equivalent shutdown support if desired

## 5.2 SPI Communication

The driver shall:

- Implement DRV8704 16-bit SPI frame transfers
- Support register reads and writes for addresses `0x00` through `0x07`
- Handle the DRV8704 convention where `SCS` is active high
- Mask register values to the 12-bit payload width
- Provide low-level methods such as:
  - `readRegister()`
  - `writeRegister()`
  - `updateRegister()`
- Provide optional readback verification after writes
- Provide SPI health check to verify that device is attached by reading registers and write/readback in the driver begin function.

## 5.3 Register Model

The driver shall represent the documented registers explicitly:

- `CTRL`
  - `ENBL`
  - `ISGAIN`
  - `DTIME`
- `TORQUE`
  - `TORQUE`
- `OFF`
  - `TOFF`
  - `PWMMODE`
- `BLANK`
  - `TBLANK`
- `DECAY`
  - `TDECAY`
  - `DECMOD`
- `DRIVE`
  - `OCPTH`
  - `OCPDEG`
  - `TDRIVEN`
  - `TDRIVEP`
  - `IDRIVEN`
  - `IDRIVEP`
- `STATUS`
  - `OTS`
  - `AOCP`
  - `BOCP`
  - `APDF`
  - `BPDF`
  - `UVLO`

The driver should use:

- symbolic register addresses
- bit masks and shifts
- typed enums for discrete settings
- optional register unions/structs for readable field access

## 5.4 Core Register Configuration

The driver shall support configuration helpers for:

- H-bridge enable/disable through `CTRL.ENBL`
- ISENSE amplifier gain selection
- Dead-time selection
- Torque DAC setting
- Fixed off-time configuration
- Current-trip blanking time configuration
- Decay mode selection
- Mixed-decay transition timing
- OCP threshold selection
- OCP deglitch time selection
- Gate drive source/sink time selection
- Gate drive source/sink current selection

The API should prefer typed setters such as:

- `setSenseGain(SenseGain gain)`
- `setDecayMode(DecayMode mode)`
- `setGateDrive(const GateDriveConfig& cfg)`

and avoid string arguments like `"slow"` or `"on"`.

## 5.5 Current Mode

The driver shall provide a high-level current-control mode that uses the DRV8704 internal current regulation.

Requirements:

- the user shall provide shunt resistor values in ohms for each bridge
- the driver shall support at least symmetric use with a single shunt value applied to both bridges
- the driver shall treat current-limit configuration as chip-global because `ISGAIN` and `TORQUE` are chip-global in the DRV8704
- the driver shall compute a suitable `ISGAIN` choice automatically from:
  - shunt resistor value
  - requested current range
  - DRV8704 current-trip equation
- the driver shall expose current commands in amperes instead of raw `TORQUE` register values
- direction selection shall be separate from current-magnitude selection
- the user shall command current in amperes
- the driver shall configure the DRV8704 so internal current chopping is used for regulation
- the driver shall select and apply timing/decay presets appropriate to the load
- the driver shall support a current-drive state where the bridge is driven statically in the currently selected direction while DRV8704 internal current regulation remains active

The current mode API should include concepts such as:

- `setShuntResistance(float ohms)`
- `setCurrentLimit(float amps)`
- `setCurrentModePreset(CurrentModePreset preset)`
- `setDirection(BridgeId bridge, Direction direction)`
- `setCurrent(BridgeId bridge, float amps)`
- `disableCurrentLimit()`

The driver shall clamp or reject impossible current requests based on:

- supported `ISGAIN`
- shunt value
- valid `TORQUE` range
- maximum practical current trip range

The driver shall report the resulting derived settings, including:

- chosen sense gain
- chosen torque DAC code
- resulting current resolution in amperes per DAC count
- achievable maximum current for the selected shunt and gain

## 5.6 Predefined Load Presets

The driver shall provide predefined timing/decay presets to simplify current-mode setup.

Required preset families:

- heater
- thermoelectric cooler
- motor, small inductance
- motor, medium inductance
- motor, large inductance

Each preset shall define recommended starting values for at least:

- `TOFF`
- `TBLANK`
- `DECMOD`
- `TDECAY`
- dead time

The preset system should allow:

- using a predefined preset directly
- starting from a preset and then overriding individual values
- adjusting preset behavior for PWM-assisted operation based on requested PWM frequency

The preset families listed above are sufficient for the first release. The implementation should prefer a small number of defensible preset families over a larger set of weakly justified variants.

## 5.7 PWM Input Mode and Platform PWM Generation

The driver shall provide a PWM-input operating mode for supported MCU platforms, but the high-level PWM mode shall always be framed as PWM with current limit rather than PWM without current limit.

Requirements:

- the driver shall generate PWM on bridge input pins using hardware timers where available
- supported initial platforms shall include:
  - ESP32
  - Teensy
  - STM
- on ESP32, the implementation should use LEDC when available
- on Teensy and STM, the implementation should use hardware PWM facilities provided by the platform core
- the user shall be able to set PWM frequency in hertz
- the driver shall enforce an upper limit based on DRV8704 input capability and timer backend capability
- the user shall be able to command speed from `0` to `100`
- the driver shall report the smallest achievable duty increment for the chosen PWM frequency and platform
- at speed `0`, the PWM input shall be off and the resulting bridge behavior shall match `coast`
- at speed `100`, the input shall be driven constantly on, not toggled as PWM
- direction selection shall be separate from speed selection
- the recommended high-level API shall not encourage PWM operation without an explicitly programmed current limit
- if the user wants effectively unbounded PWM drive, that should require deliberate programming of a very high current limit rather than a separate “unsafe” PWM mode
- PWM mode shall support use with DRV8704 current limiting active during PWM on-time
- load presets may adjust timing and decay values based on PWM frequency as well as load class

The PWM mode API should include concepts such as:

- `beginPwmMode(const PwmConfig& cfg)`
- `setPwmFrequency(uint32_t hz)`
- `setSpeed(BridgeId bridge, float percent)`
- `setDirection(BridgeId bridge, Direction direction)`
- `getDutyStepPercent()`
- `getDutyStepCounts()`

The driver shall document that this is open-loop duty control, not closed-loop speed regulation.

## 5.8 Runtime Bridge Control

Because bridge switching is controlled by `AIN1/AIN2/BIN1/BIN2`, the driver should optionally provide GPIO helpers for both bridges:

- Coast
- Brake
- Forward
- Reverse

The runtime bridge-control layer shall make the following high-level states explicit:

- `Coast`
- `Brake`
- `CurrentDrive`
- `PwmDriveWithCurrentLimit`

Behavioral expectations:

- `Coast` and `Brake` are immediate per-bridge state overrides
- on a given bridge, `Coast` and `Brake` are mutually exclusive with active current-drive and PWM-drive commands
- direction shall be commanded independently of magnitude commands
- `setCurrent(...)` and `setSpeed(...)` shall not change direction
- `setDirection(...)` shall define the active forward/reverse orientation used by both current-drive and PWM-drive commands
- the most recent bridge command on a bridge wins
- `setSpeed(..., 0)` shall behave like `coast(...)`

This runtime control should be separate from SPI configuration.

Suggested abstraction:

- A per-bridge command API with explicit mode transitions
- Or a chip-level API with `BridgeA` and `BridgeB` selectors

## 5.9 Fault and Status Handling

The driver shall:

- Read and decode the `STATUS` register
- Provide a typed fault/status structure
- Detect whether any fault is active
- Support clearing clearable latched faults by writing zero to relevant bits
- Respect datasheet behavior for auto-clearing conditions such as thermal shutdown and UVLO recovery
- Optionally monitor the `FAULTn` pin when provided

The driver should provide helpers such as:

- `readStatus()`
- `hasFault()`
- `clearFaults()`
- `clearFault(FaultBit bit)`

## 5.10 Configuration Profiles

The driver should provide a top-level configuration structure that captures the device operating profile, for example:

- sense gain
- dead time
- torque
- off time
- blanking time
- decay mode
- decay time
- OCP threshold and deglitch
- gate-drive current and timing
- initial bridge enable state

This enables:

- `applyConfig(const DRV8704Config& cfg)`
- `readConfigSnapshot()`
- possible future serialization or diagnostics

This should be extended with higher-level mode configuration structures such as:

- `CurrentModeConfig`
- `PwmConfig`
- `LoadPresetConfig`

## 5.11 Diagnostics and Validation

The driver should provide:

- register dump support
- startup identification of SPI communication success
- readback checking of configuration writes
- sanity checking on reserved bits
- simple health report structure

For current mode and PWM mode diagnostics, the driver should also report:

- chosen gain and torque settings derived from current requests
- PWM frequency actually achieved by the timer backend
- duty-cycle resolution and smallest achievable increment

## 5.12 Documentation Requirements

The project shall include a `README.md` in the project root.

The README shall explain:

- the purpose of the DRV8704 library
- the role and limitations of the DRV8704 chip
- the purpose of the main driver functions
- the difference between SPI configuration functions and GPIO bridge-control functions
- required hardware connections
- expected startup and usage flow
- example usage for initialization, configuration, and bridge control
- fault and status handling concepts

The project shall also support Doxygen documentation generation.

Requirements:

- public classes, structs, enums, typedefs, and public functions shall have Doxygen-compatible comment headers
- nontrivial internal helper functions should also be documented where useful
- function comments should include parameter, return, and behavioral notes where relevant
- documentation format should be consistent across the project
- a project `Doxyfile` shall be included
- the generated documentation output location shall be defined in the project

Recommended Doxygen tags:

- `@brief`
- `@param`
- `@return`
- `@details`
- `@ingroup`

## 5.13 Documentation Scripts and Tooling

The DRV8704 project shall reuse the MAX30001G documentation/release tooling pattern.

Requirements:

- copy relevant scripts from `~/Documents/GitHub/BioMedicalSensorBoard/Arduino/libraries/UUtzinger_MAX30001G/scripts`
- modify those scripts for DRV8704 names, files, assets, and output paths
- copy and adapt the MAX30001G `Doxyfile` for this project
- ensure documentation generation works with the DRV8704 source layout and project files

Expected scripts to copy and adapt:

- `scripts/generate_docs.sh`
- `scripts/release_from_library_version.sh`

## 5.14 Arduino Library Packaging Requirements

The project shall include the standard Arduino library files needed for GitHub hosting and Arduino library registration.

Required packaging files and folders:

- `library.properties`
- `keywords.txt`
- `README.md`
- `src/`
- `examples/`
- license file such as `LICENSE` or `License.txt`

Recommended additional project files:

- `CHANGELOG.md`
- `Doxyfile`
- `scripts/`
- `assets/`

The Arduino metadata shall clearly distinguish repository naming from library naming:

- GitHub repository name: `Arduino_DRV8704`
- Arduino library name for registration and `library.properties`: `UUtzinger_DRV8704`

The `library.properties` file shall:

- use `name=UUtzinger_DRV8704`
- use the GitHub URL for the `Arduino_DRV8704` repository
- declare the public include header for the library
- use an Arduino category appropriate for a motor-driver library
- list any required dependencies explicitly

## 6. API Requirements

The public API should be Arduino-friendly and stable.

Minimum expected public surface:

- constructor with pin configuration
- `begin()`
- `sleep()`
- `wake()`
- `reset()`
- `enableBridgeDriver(bool enabled)`
- `applyConfig(...)`
- register read/write helpers
- status/fault helpers
- optional bridge command helpers
- current-limit helpers that accept current in amperes
- bridge direction helper
- PWM-with-current-limit helpers that accept frequency and speed in percent on supported platforms
- explicit `coast` and `brake` helpers or equivalent bridge-state commands

Minimum expected high-level public capabilities:

- current-drive mode:
  - set shunt resistance
  - choose load preset
  - set current limit in amps
  - set direction
  - enter current-drive state
- PWM-with-current-limit mode:
  - set PWM frequency
  - set direction
  - set speed `0..100`
  - report smallest achievable duty increment
  - force coast at `0`
  - force constant on at `100`
- static bridge control:
  - coast
  - brake

The API should avoid:

- raw mutable public member variables
- string-based command inputs
- duplicated constants between header and source
- exposing reserved register writes to sketches by default
- encouraging PWM-without-current-limit as a primary operating mode

## 7. Proposed Source Structure

The structure should follow the style of `UUtzinger_MAX30001G`, meaning one main public header plus smaller headers and focused implementation units.

Recommended replacement layout:

```text
README.md
CHANGELOG.md
library.properties
keywords.txt
Doxyfile
scripts/
  generate_docs.sh
  release_from_library_version.sh
src/
  drv8704.h
  drv8704_defs.h
  drv8704_types.h
  drv8704_regs_typedefs.h
  drv8704_comm.cpp
  drv8704.cpp
  drv8704_config.cpp
  drv8704_current.cpp
  drv8704_status.cpp
  drv8704_bridge.cpp
  drv8704_pwm.h
  drv8704_pwm_general.cpp
  drv8704_pwm_esp32.cpp
  drv8704_pwm_teensy.cpp
  drv8704_pwm_stm.cpp
examples/
```

### 7.1 File Responsibilities

`drv8704.h`

- Main public class declaration
- Public enums and API surface if not moved to `drv8704_types.h`
- Minimal includes

`drv8704_defs.h`

- Register addresses
- bit masks
- shifts
- SPI speed/settings constants
- datasheet constants and default reset values

`drv8704_types.h`

- configuration structs
- status/fault structs
- bridge mode enums
- result and health-check types
- current-mode and PWM-mode config/result types

`drv8704_regs_typedefs.h`

- typed register unions/bitfields for 12-bit control/status registers
- one union per register where useful

`drv8704_comm.cpp`

- SPI transport
- frame formatting
- raw register read/write/update helpers

`drv8704.cpp`

- constructor
- `begin()`
- `sleep()`
- `wake()`
- `reset()`
- top-level orchestration

`drv8704_config.cpp`

- all configuration setters/getters
- `applyConfig()`
- config snapshot helpers

`drv8704_current.cpp`

- shunt-aware current-mode helpers
- gain and torque derivation from requested current
- predefined load preset mapping
- current-limit enable/disable helpers
- interaction between current-drive mode and PWM-with-current-limit mode

`drv8704_status.cpp`

- status register decode
- fault clear logic
- register dump and health-check helpers

`drv8704_bridge.cpp`

- GPIO-level bridge control helpers for A and B bridges

`drv8704_pwm.h`

- abstract interface and shared types for platform PWM generation

`drv8704_pwm_esp32.cpp`

- ESP32 LEDC-based PWM generator backend when available

`drv8704_pwm_teensy.cpp`

- Teensy hardware-PWM backend

`drv8704_pwm_stm.cpp`

- STM hardware-PWM backend

`drv8704_pwm_general.cpp`

- generic Arduino `analogWrite()` fallback backend

`README.md`

- top-level library overview
- purpose of the main driver functions
- quick-start usage and wiring notes

`Doxyfile`

- Doxygen configuration adapted from the MAX30001G project
- inputs updated for DRV8704 source and markdown files

`scripts/generate_docs.sh`

- generates DRV8704 documentation using Doxygen
- mirrors required assets into generated docs if needed

`scripts/release_from_library_version.sh`

- release helper adapted from MAX30001G
- optionally refreshes docs as part of release flow

## 8. Naming Requirements

Recommended naming:

- Class name: `DRV8704`
- GitHub repository name: `Arduino_DRV8704`
- Arduino library name: `UUtzinger_DRV8704`
- Header guard and file names should match the final class name consistently
- Register types should use a clear suffix such as `_reg_t`
- Current-mode and PWM-mode types should clearly distinguish raw register values from user-facing amperes, hertz, and percent

Avoid:

- lowercase class name `drv`
- inconsistent constructor signatures
- generic file names like `drv.cpp` and `drv.h`

## 9. Behavioral Requirements

The implementation shall:

- preserve reserved bits when updating a register
- write only documented values for enumerated fields
- clamp or reject invalid numeric inputs
- document units for all timing/current helper APIs
- document units for all timing/current/PWM helper APIs
- use Doxygen-compatible headers on public API functions
- separate chip state from hardware pin control state
- support use without all optional pins connected
- separate generic DRV8704 logic from ESP32/Teensy-specific PWM backend code
- map speed `0` to coast and speed `100` to constant-on bridge command with no PWM toggling
- provide a clear direction model separate from current and speed magnitude commands
- make explicit how the user transitions between `Coast`, `Brake`, `CurrentDrive`, and `PwmDriveWithCurrentLimit`
- avoid encouraging PWM-without-current-limit in the high-level API
- document that DRV8704 current-limit settings remain active when PWM drive is used unless explicitly changed

## 10. Known Issues in Existing Attempt

The current `src/drv.h` and `src/drv.cpp` show several reasons to replace the implementation instead of reusing it directly:

- constructor declarations and definitions do not match
- public members expose internal state directly
- array members are declared without valid sizes in the class definition
- string comparison patterns are not robust for command APIs
- constants are duplicated in both header and source
- design is register-centric but not structured around typed register definitions
- there is no clear separation between SPI transport, configuration, and status handling

## 11. Development Plan

## Phase 1: Specification Freeze

- Confirm public class name
- Confirm pin model
- Confirm whether bridge GPIO helpers are required in the first implementation
- Freeze the initial config struct and enum names
- Freeze current-mode preset definitions
- Freeze supported PWM backends for first release
- Freeze the bridge state model and mode-transition semantics

Deliverable:

- This requirements document

## Phase 2: Source Skeleton

- Replace `drv.h` and `drv.cpp` with the new file structure
- Create `README.md`
- Create `library.properties`
- Create `keywords.txt`
- Create `examples/`
- Copy and adapt `Doxyfile` from MAX30001G
- Copy and adapt `scripts/generate_docs.sh`
- Copy and adapt `scripts/release_from_library_version.sh`
- Create `drv8704_defs.h`
- Create `drv8704_types.h`
- Create `drv8704_regs_typedefs.h`
- Create PWM backend source files for ESP32 and Teensy
- Create the main public header

Deliverable:

- Buildable library skeleton with documentation tooling scaffold

## Phase 3: SPI and Register Layer

- Implement SPI frame transfer
- Implement raw register read/write/update functions
- Add register reset defaults
- Add register dump helper

Deliverable:

- Verified register communication layer

## Phase 4: Configuration Layer

- Implement typed setters/getters for all documented configuration fields
- Implement `applyConfig()`
- Add readback validation where appropriate
- Implement current-mode helpers that derive gain and torque from shunt value and current target
- Implement predefined load presets
- Implement PWM-with-current-limit helpers and frequency/resolution reporting
- Implement explicit direction, coast, and brake semantics

Deliverable:

- Full configuration API

## Phase 5: Status and Fault Layer

- Implement status decode
- Implement fault clear functions
- Implement health-check helper
- Integrate optional `FAULTn` polling

Deliverable:

- Full diagnostics/fault API

## Phase 6: Bridge Control Layer

- Implement GPIO helpers for bridge A and B
- Define forward, reverse, brake, and coast semantics
- Define explicit mode transitions between current-drive, PWM-with-current-limit, coast, and brake
- Document expected PWM usage model
- Implement platform PWM backends for ESP32, Teensy, STM, and generic Arduino fallback
- Map speed `0..100` onto hardware PWM plus coast/constant-on edge cases

Deliverable:

- Runtime bridge-control API

## Phase 7: Examples and Validation

- Add a basic initialization example
- Add a register dump / health-check example
- Add a bridge control example
- Add a current-mode example using shunt-aware amperes input
- Add a PWM-mode example for supported boards
- Generate docs using the adapted Doxygen workflow
- Validate behavior on hardware

Deliverable:

- Usable examples, generated docs, and bring-up workflow

## 12. Acceptance Criteria

The first replacement driver is acceptable when:

- it compiles as an Arduino library
- it includes standard Arduino library packaging files such as `library.properties`, `keywords.txt`, `src/`, and `examples/`
- all documented DRV8704 registers are represented cleanly
- configuration APIs use typed inputs instead of strings
- runtime bridge pin control is clearly separated from SPI configuration
- the driver provides a current-drive mode where the user specifies shunt resistance, direction, and current limit in amperes
- the driver automatically derives suitable DRV8704 current-regulation settings from that current-limit request
- the driver provides predefined load presets for heater, thermoelectric cooler, and motor inductance classes
- the driver provides PWM-with-current-limit support for ESP32, Teensy, STM, and generic Arduino PWM backends
- the driver allows the user to command speed from `0` to `100`, maps `0` to coast, and reports achievable duty resolution
- the driver exposes explicit `coast` and `brake` bridge states
- the driver uses a direction model separate from current and speed magnitude commands
- status and faults can be read and cleared correctly
- the project includes a `README.md` explaining the purpose of the main driver functions
- public APIs use Doxygen-compatible function headers
- the GitHub repository naming and Arduino library naming are separated correctly:
  - repository name `Arduino_DRV8704`
  - library name `UUtzinger_DRV8704`
- MAX30001G documentation scripts and `Doxyfile` have been copied and adapted for DRV8704
- the library structure is modular and matches the example style closely enough to be maintainable
- the old `drv` implementation is no longer needed

## 13. Recommended Next Step

Next, use this document to create the replacement source skeleton in `src/`, starting with:

- `drv8704.h`
- `drv8704_defs.h`
- `drv8704_types.h`
- `drv8704_regs_typedefs.h`
- `drv8704_comm.cpp`
- `drv8704.cpp`
