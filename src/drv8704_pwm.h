/**
 * @file drv8704_pwm.h
 * @brief Shared abstraction for platform PWM generation backends.
 */

#ifndef DRV8704_PWM_H
#define DRV8704_PWM_H

#include <Arduino.h>

#include "drv8704_types.h"

/**
 * @brief Abstract board-specific PWM backend used for DRV8704 bridge inputs.
 */
class DRV8704PwmBackend {
public:
  virtual ~DRV8704PwmBackend() {}

  /**
   * @brief Initialize hardware PWM resources for the provided input pins.
   * @param pins Driver pin bundle.
   * @param config Requested PWM configuration.
   * @param capability Resolved PWM capability report.
   * @return True when the backend is available and initialized.
   */
  virtual bool begin(const DRV8704Pins& pins,
                     const DRV8704PwmConfig& config,
                     DRV8704PwmCapability& capability) = 0;

  /**
   * @brief Release hardware PWM resources and detach all PWM pins.
   * @param pins Driver pin bundle.
   */
  virtual void end(const DRV8704Pins& pins) = 0;

  /**
   * @brief Update the operating PWM frequency.
   * @param pins Driver pin bundle.
   * @param config Requested PWM configuration.
   * @param capability Updated PWM capability report.
   * @return True when the backend accepted the requested frequency.
   */
  virtual bool configure(const DRV8704Pins& pins,
                         const DRV8704PwmConfig& config,
                         DRV8704PwmCapability& capability) = 0;

  /**
   * @brief Apply a PWM duty count to one bridge-input pin.
   * @param pin MCU pin number.
   * @param dutyCount Duty count from 0 to max-duty.
   * @param maxDuty Maximum duty count supported by the active resolution.
   * @return True when the PWM output was applied.
   */
  virtual bool writeDuty(int8_t pin, uint32_t dutyCount, uint32_t maxDuty) = 0;

  /**
   * @brief Stop PWM on one pin and return it to GPIO control.
   * @param pin MCU pin number.
   */
  virtual void releasePin(int8_t pin) = 0;
};

/**
 * @brief Create the platform PWM backend selected by the active Arduino core.
 * @return Backend instance or `nullptr` when the target has no supported PWM backend.
 */
DRV8704PwmBackend* drv8704CreatePwmBackend();

#endif // DRV8704_PWM_H
