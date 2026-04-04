# Examples

The example set currently includes:

- `BasicBringup`
- `RegisterHealthCheck`
- `BridgeControlDemo`
- `CurrentModeDemo`
- `CurrentPresetDemo`
- `PwmModeDemo`

The recommended workflow is now:

- `CurrentModeDemo` for static current drive with explicit direction
- `CurrentPresetDemo` for preset-based current-limit tuning
- `PwmModeDemo` for PWM with current limit

All examples use placeholder pin assignments. Adjust the pin numbers to match your board and DRV8704 hardware before uploading.
