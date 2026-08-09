# Stage 7: Hysteretic state classifier

## Goal

Convert one-second vibration RMS windows into a stable bench `RUN` or `STOP` state without allowing a single impact or threshold-edge noise to change state.

## Bench classifier

| Rule | Value |
|---|---:|
| Enter `RUN` | RMS at or above 0.120g for 2 consecutive valid windows |
| Enter `STOP` | RMS at or below 0.060g for 2 consecutive valid windows |
| Hysteresis band | Above 0.060g and below 0.120g retains the current state |
| Startup | `UNKNOWN` until two consecutive windows support a state |
| Invalid sample window | Retain state and reset pending evidence |
| Serial baud rate | 115200 baud |

The thresholds bracket the Stage 6 bench midpoint candidate of 0.11896g while leaving a broad hysteresis band above the measured stationary range. They are bench-test values only.

At each boot, a deterministic synthetic sequence verifies the confirmation and hysteresis rules before sensor calibration. Any self-test failure halts the firmware.

## Pass criteria

- On startup while stationary, state transitions `UNKNOWN -> STOP` only after two complete low-RMS windows.
- One isolated high-RMS window does not change `STOP` to `RUN`.
- Sustained deliberate movement transitions `STOP -> RUN` only after two complete high-RMS windows.
- After movement stops, state transitions `RUN -> STOP` only after two complete low-RMS windows.
- All evidence windows contain 200/200 samples, no read failures, no missed periods, and 199.0 Hz to 201.0 Hz sampling.

Do not use this bench classifier as a machine-state detector until Stage 8 replaces or validates its thresholds using actual mounted equipment data.
