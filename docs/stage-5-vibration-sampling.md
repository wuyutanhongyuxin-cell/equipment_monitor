# Stage 5: Vibration sampling and filtering baseline

## Goal

Establish a repeatable signal-processing baseline before choosing any RUN/STOP threshold. This stage verifies sampling timing, I2C reliability, gravity removal, and stationary vibration noise. It does not classify equipment state.

## Signal design

| Parameter | Value | Reason |
|---|---:|---|
| Sample rate | 200 Hz | Captures vibration content up to the filtered Nyquist limit while leaving ample ESP32 processing margin |
| I2C clock | 50 kHz | Conservative rate for the verified breadboard wiring while retaining enough bus capacity for 200 Hz reads |
| MPU6050 DLPF | approximately 44 Hz | Reduces high-frequency noise and prevents using frequencies too close to the 100 Hz Nyquist limit |
| Accelerometer range | +/-2g | Maximizes resolution during bench bring-up |
| Gravity baseline | 0.5 Hz low-pass | Tracks slow orientation changes while preserving faster vibration |
| Statistics window | 1 second / 200 attempts | Produces stable, interpretable RMS and peak values |
| Startup calibration | 400 valid samples / 2 seconds | Initializes the gravity estimate while stationary |

The scalar vibration metric is the magnitude of the three-axis acceleration after subtracting the low-pass gravity baseline. The firmware reports RMS and peak magnitude in g for each window.

Initialization permits at most five identity/read-back attempts, separated by 20 ms. Each rejected value is reported. Sampling starts only after `WHO_AM_I`, DLPF, sample divider, and accelerometer range values are read back correctly; persistent initialization errors stop execution.

The tested GY-521 consistently returns non-standard `WHO_AM_I=0x74`, although the official MPU-6050 value is `0x68`. The firmware accepts `0x74` only as an explicitly reported compatible-device identity, then requires configuration-register read-back and plausible acceleration behavior. This does not establish that the fitted chip is an authentic TDK/InvenSense MPU-6050.

## Test procedure

1. Keep the Stage 3 wiring unchanged and place the breadboard on a stationary surface.
2. Upload `firmware/stage_05_vibration_sampling/stage_05_vibration_sampling.ino` to `ESP32 Dev Module` on `COM6`.
3. Open Serial Monitor at 115200 baud.
4. Do not touch the sensor during the two-second calibration.
5. Capture at least 30 consecutive one-second windows while stationary.

## Expected output

```text
ESP32 vibration sampling test boot OK
MPU6050 init: ready, sample_target_hz=200, dlpf_hz=44, accel_range=+/-2g
Calibration: keep sensor stationary, samples=400
Calibration: ready, gravity_g=(-0.1200,-0.0200,+1.0200), failures=0
window: rate_hz=200.00, valid=200/200, failed=0, missed=0, vibration_rms_g=0.00500, peak_g=0.01500
```

## Pass criteria

- Identity is explicitly reported as standard `0x68` or observed compatible `0x74`, all configuration read-backs pass, and the 400-sample calibration completes.
- At least 30 consecutive windows are captured.
- Every window has `valid=200/200`, `failed=0`, and `missed=0`.
- Reported sampling rate remains between 199.0 Hz and 201.0 Hz.
- Stationary RMS and peak values are recorded as a baseline; no universal noise threshold is assumed in advance.

Only after these criteria pass may Stage 6 collect contrasting stationary and deliberately moved/vibrating datasets for threshold design.
