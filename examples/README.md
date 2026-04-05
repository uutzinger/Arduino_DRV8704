# Examples

The example set currently includes:

- `BasicBringup`
- `RegisterHealthCheck`
- `DRV8704_Test`
- `Current_Test`
- `PWM_Test`
- `BridgeControlDemo`
- `CurrentModeDemo`
- `CurrentPresetDemo`
- `PwmModeDemo`

The recommended workflow is now:

- `BasicBringup` first, to confirm the board powers up, SPI is wired correctly, and the driver responds
- `RegisterHealthCheck` next, to verify register access and spot configuration or fault issues early
- `DRV8704_Test` after bring-up, for interactive engineering tests covering current drive, PWM drive, full bridge drive, and current-mode timing sweeps
- `CurrentModeDemo` for static current drive with explicit direction
- `CurrentPresetDemo` for preset-based current-limit tuning
- `Current_Test` for interactive serial testing, mode switching, and timing sweeps on real hardware
- `PwmModeDemo` for PWM with current limit
- `PWM_Test` for interactive serial PWM testing with current-limit, frequency, resolution, and full-bridge comparison

All examples use placeholder pin assignments. Adjust the pin numbers to match your board and DRV8704 hardware before uploading.
