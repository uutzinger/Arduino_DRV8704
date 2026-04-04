# DRV8704 Driver TODO

This file converts `DRV8704_DRIVER_REQUIREMENTS.md` into an implementation checklist.

## 1. Project Skeleton

- [x] Create `README.md`
- [x] Create `library.properties`
- [x] Create `keywords.txt`
- [x] Create `Doxyfile`
- [x] Create `scripts/generate_docs.sh`
- [x] Create `scripts/release_from_library_version.sh`
- [x] Create `examples/`
- [x] Replace the legacy `src/drv.h` and `src/drv.cpp` layout with the new `src/` structure

## 2. Core Source Layout

- [x] Create `src/drv8704.h`
- [x] Create `src/drv8704_defs.h`
- [x] Create `src/drv8704_types.h`
- [x] Create `src/drv8704_regs_typedefs.h`
- [x] Create `src/drv8704_comm.cpp`
- [x] Create `src/drv8704.cpp`
- [x] Create `src/drv8704_config.cpp`
- [x] Create `src/drv8704_status.cpp`
- [x] Create `src/drv8704_bridge.cpp`
- [x] Create `src/drv8704_current.cpp`
- [x] Create `src/drv8704_pwm.h`
- [x] Create `src/drv8704_pwm_general.cpp`
- [x] Create `src/drv8704_pwm_esp32.cpp`
- [x] Create `src/drv8704_pwm_teensy.cpp`
- [x] Create `src/drv8704_pwm_stm.cpp`

## 3. Register Definitions

- [x] Define register addresses
- [x] Define register reset/default values
- [x] Define bit masks and shifts
- [x] Define SPI constants and settings
- [x] Define enums for discrete register field values

## 4. Types and Register Views

- [x] Define configuration structs
- [x] Define status/fault structs
- [x] Define bridge-control enums
- [x] Define health-check/result structs
- [x] Define typed register unions/bitfields where useful
- [x] Define current-mode config structs
- [x] Define load-preset config structs
- [x] Define PWM config structs
- [x] Define PWM capability/result structs
- [x] Define direction enum for high-level current/PWM commands
- [x] Define explicit bridge-state type for:
  - `Coast`
  - `Brake`
  - `CurrentDrive`
  - `PwmDriveWithCurrentLimit`
- [x] Define chip-global current-limit config/result structs
- [x] Define per-bridge direction and mode-state reporting structs

## 5. Public API

- [x] Define the `DRV8704` class in `src/drv8704.h`
- [x] Define constructor and pin model
- [x] Define lifecycle API: `begin()`, `sleep()`, `wake()`, `reset()`
- [x] Define SPI register API: `readRegister()`, `writeRegister()`, `updateRegister()`
- [x] Define configuration API: typed setters and `applyConfig()`
- [x] Define status/fault API
- [x] Define optional bridge-control API
- [x] Define current-mode API:
  - set shunt resistance
  - set preset
  - set current in amps
  - set direction
  - disable current mode
- [x] Define PWM-mode API:
  - begin PWM mode
  - set PWM frequency
  - set speed `0..100`
  - report smallest duty increment
  - report achieved PWM frequency
- [x] Refactor to explicit direction API:
  - `setDirection(BridgeId bridge, Direction direction)`
- [x] Refactor current-limit API to chip-global semantics:
  - `setCurrentLimit(float amps)`
  - `disableCurrentLimit()`
- [x] Refactor current-drive API so current magnitude does not change direction:
  - `setCurrent(BridgeId bridge, float amps)`
- [x] Refactor PWM API so speed magnitude does not change direction:
  - `setSpeed(BridgeId bridge, float percent)`
- [x] Expose explicit `coast(BridgeId bridge)` and `brake(BridgeId bridge)` helpers or equivalent bridge-state commands
- [x] Document and enforce last-command-wins semantics per bridge

## 6. SPI Transport

- [x] Implement raw 16-bit DRV8704 SPI frame transfer
- [x] Implement active-high `SCS` handling
- [x] Implement register read support
- [x] Implement register write support
- [x] Implement masked register update support
- [x] Implement SPI health check

## 7. Lifecycle and Startup

- [x] Implement constructor
- [x] Implement `begin()`
- [x] Configure pin modes
- [x] Initialize SPI
- [x] Wake device if `SLEEPn` is available
- [x] Reset device if `RESET` is available
- [x] Read and cache startup registers

## 8. Core Register Configuration Layer

- [x] Implement H-bridge enable control through `CTRL.ENBL`
- [x] Implement ISENSE gain selection
- [x] Implement dead-time selection
- [x] Implement torque DAC setting
- [x] Implement fixed off-time configuration
- [x] Implement blanking-time configuration
- [x] Implement decay mode selection
- [x] Implement mixed-decay timing
- [x] Implement OCP threshold selection
- [x] Implement OCP deglitch selection
- [x] Implement gate-drive source/sink timing
- [x] Implement gate-drive source/sink current
- [x] Implement `applyConfig()`
- [x] Implement readback validation where appropriate

## 9. Status and Fault Layer

- [x] Implement `STATUS` register decode
- [x] Implement typed fault/status result structure
- [x] Implement `hasFault()`
- [x] Implement `readStatus()`
- [x] Implement `clearFault(FaultBit bit)`
- [x] Implement `clearFaults()`
- [x] Handle optional `FAULTn` monitoring

## 10. Runtime Bridge Control Layer

- [x] Implement bridge A control helpers
- [x] Implement bridge B control helpers
- [x] Implement coast mode
- [x] Implement brake mode
- [x] Implement forward mode
- [x] Implement reverse mode
- [x] Document expected PWM usage model
- [x] Refactor bridge control around explicit runtime states:
  - `Coast`
  - `Brake`
  - `CurrentDrive`
  - `PwmDriveWithCurrentLimit`
- [x] Ensure `Coast` and `Brake` immediately override active current-drive and PWM-drive commands on the same bridge
- [x] Ensure `setSpeed(..., 0)` behaves exactly like `coast(...)`
- [x] Ensure direction is not changed by `setCurrent(...)` or `setSpeed(...)`

## 11. Current Limit and Current Drive

- [x] Implement shunt-aware current mode in `src/drv8704_current.cpp`
- [x] Support one shunt value for both bridges
- [x] Support independent shunt values per bridge
- [x] Compute suitable `ISGAIN` automatically from shunt value and current target
- [x] Compute `TORQUE` DAC code automatically from current target
- [x] Clamp or reject impossible current requests
- [x] Allow user to set current in amperes
- [x] Allow user to set direction:
  - forward
  - reverse
- [x] Allow current mode to engage bridge drive without requiring external PWM
- [x] Report derived settings:
  - selected gain
  - torque DAC code
  - current resolution in amps/count
  - achievable max current
- [x] Refactor current limit to chip-global behavior in the public API
- [x] Separate current-limit programming from direction changes
- [x] Support explicit current-drive state that uses the currently selected direction
- [x] Implement `disableCurrentLimit()` semantics without forcing a bridge-state change
- [x] Document that current-limit settings remain active during PWM drive unless explicitly changed

## 12. Predefined Load Presets

- [x] Define preset family for heater
- [x] Define preset family for thermoelectric cooler
- [x] Define preset family for motor, small inductance
- [x] Define preset family for motor, medium inductance
- [x] Define preset family for motor, large inductance
- [x] Map each preset to recommended:
  - `TOFF`
  - `TBLANK`
  - `DECMOD`
  - `TDECAY`
  - dead time
- [x] Support using presets directly
- [x] Support overriding individual fields after preset selection
- [x] Adjust preset behavior for PWM-assisted operation based on requested PWM frequency
- [x] Document why preset timing changes between current-drive and PWM-with-current-limit operation

## 13. PWM With Current Limit and Platform Backends

- [x] Create shared PWM backend abstraction in `src/drv8704_pwm.h`
- [x] Implement general Arduino PWM backend in `src/drv8704_pwm_general.cpp`
- [x] Implement ESP32 PWM backend in `src/drv8704_pwm_esp32.cpp`
- [x] Use LEDC on ESP32 when available
- [x] Implement Teensy PWM backend in `src/drv8704_pwm_teensy.cpp`
- [x] Implement STM PWM backend in `src/drv8704_pwm_stm.cpp`
- [x] Use `analogWrite()` as a generic fallback backend
- [x] Use platform hardware PWM facilities on Teensy
- [x] Use platform hardware PWM facilities on STM
- [x] Support configurable PWM frequency in hertz
- [x] Enforce PWM frequency limit from driver capability and backend capability
- [x] Report achieved PWM frequency
- [x] Report smallest duty increment
- [x] Support speed command from `0` to `100`
- [x] Map speed `0` to explicit `coast`
- [x] Map speed `100` to constant-on input state with no PWM toggling
- [x] Document max frequency limits and platform differences
- [x] Refactor PWM mode into explicit `PwmDriveWithCurrentLimit` high-level behavior
- [x] Remove or de-emphasize high-level PWM-without-current-limit usage in the API
- [x] Separate direction selection from speed selection
- [x] Ensure PWM examples and docs describe current-limited PWM as the primary use pattern
- [x] Update speed-0 semantics from “fully off input state” to explicit `coast`

## 14. Documentation

- [x] Write `README.md` with library purpose and function overview
- [x] Document the difference between SPI configuration and GPIO bridge control
- [x] Add quick-start wiring notes
- [x] Add example usage snippets
- [x] Add Doxygen-compatible headers to public classes
- [x] Add Doxygen-compatible headers to public structs and enums
- [x] Add Doxygen-compatible headers to public functions
- [x] Add Doxygen headers to important internal helpers where useful
- [x] Adapt `Doxyfile` from MAX30001G
- [x] Adapt doc-generation scripts from MAX30001G
- [x] Document current mode:
  - shunt usage
  - current equation
  - preset selection
- [x] Document PWM mode:
  - ESP32 backend
  - Teensy backend
  - STM backend
  - frequency/resolution tradeoffs
- [x] Document current-mode vs PWM-mode application scenarios
- [x] Rewrite README around the revised control model:
  - direction separate from magnitude
  - `Coast`
  - `Brake`
  - `CurrentDrive`
  - `PwmDriveWithCurrentLimit`
- [x] Document how to transition between:
  - coast
  - brake
  - current-drive
  - PWM-with-current-limit
- [x] Document chip-global current-limit behavior clearly
- [x] Re-read and clean up the README after the refactor is complete

## 15. Arduino Packaging

- [x] Set `library.properties` name to `UUtzinger_DRV8704`
- [x] Set repository URL to `Arduino_DRV8704`
- [x] Set Arduino category
- [x] Set public include header
- [x] Declare dependencies if needed
- [x] Populate `keywords.txt`
- [x] Add license file if missing

## 16. Examples

- [x] Create a minimal initialization example
- [x] Create a register dump / health-check example
- [x] Create a bridge-control example
- [x] Create a current-mode example using shunt resistance and amps
- [x] Create a preset-based example for heater or thermoelectric load
- [x] Create a PWM-mode example
- [x] Refactor current-drive example to use:
  - `setDirection(...)`
  - chip-global current limit
  - `setCurrent(...)` without direction
- [x] Refactor PWM example to use:
  - `setDirection(...)`
  - `setCurrentLimit(...)`
  - `setSpeed(...)` without direction
- [x] Add example showing transition between:
  - PWM-with-current-limit
  - coast
  - brake
  - current-drive

## 17. Validation

- [x] Verify the library compiles as an Arduino library
- [x] Verify all documented registers are represented
- [ ] Verify typed APIs replace string-based APIs
- [ ] Verify status and fault handling behavior
- [ ] Verify current-mode gain selection and torque derivation
- [ ] Verify preset selection produces sensible register settings
- [ ] Verify PWM backends compile on:
  - ESP32
  - Teensy
  - STM
- [ ] Verify PWM frequency limit handling
- [ ] Verify speed `0` and `100` edge cases
- [ ] Verify direction changes occur only through the explicit direction API
- [ ] Verify coast/brake override active PWM and current-drive commands immediately
- [ ] Verify current-limit settings remain active during PWM drive
- [ ] Verify `disableCurrentLimit()` behavior
- [ ] Verify preset adjustments remain sensible across PWM frequencies
- [ ] Generate docs with the adapted Doxygen workflow
- [ ] Validate behavior on hardware

## 18. Implementation Order

1. Preserve the working base driver and completed examples. `[done]`
2. Add shared PWM types to `src/drv8704_types.h`. `[done]`
3. Extend the public API for PWM mode in `src/drv8704.h`. `[done]`
4. Implement shared and platform PWM backends. `[done]`
5. Add PWM examples and documentation. `[done]`
6. Add current-mode and preset types to `src/drv8704_types.h`. `[done]`
7. Extend the public API for current mode in `src/drv8704.h`. `[done]`
8. Implement current mode in `src/drv8704_current.cpp`. `[done]`
9. Implement preset mapping and preset override workflow. `[done]`
10. Add current-mode examples. `[done]`
11. Update documentation for current mode and presets. `[done]`
12. Refactor the API around explicit direction and bridge-state semantics. `[done]`
13. Refactor current limit to chip-global high-level behavior. `[done]`
14. Refactor PWM into explicit PWM-with-current-limit behavior. `[done]`
15. Update presets for PWM-frequency-aware tuning. `[done]`
16. Rework examples around the new control model. `[done]`
17. Re-read and clean up the README after implementation stabilizes. `[done]`
18. Run compile and hardware validation on supported targets.
