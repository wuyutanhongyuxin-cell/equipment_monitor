# ESP32 Industrial Equipment Monitor

Low-voltage, external monitor for industrial equipment. The planned MVP uses an ESP32 and an MPU6050 to detect machine vibration, classify RUN/STOP state, and send phone alerts without modifying the monitored machine.

## Safety boundary

- This project is external and low voltage only.
- Do not connect the monitor to 220 V/380 V power, internal machine terminals, spindle-drive power, or servo-drive power.
- Stage 0/1 uses only the ESP32 and its USB connection. Do not connect the MPU6050 yet.

## Current progress

| Stage | Scope | Status |
|---|---|---|
| 0 | Arduino IDE and ESP32 board support | Passed on hardware (2026-08-08) |
| 1 | Compile, upload, and serial output | Passed on hardware (2026-08-08) |
| 2 | Non-blocking WiFi connection test | Passed on hardware (2026-08-09) |
| 3 | MPU6050 I2C diagnostic scanner | Passed on hardware (2026-08-09) |
| 4 | MPU6050 acceleration reading | Passed on hardware (2026-08-09) |
| 5 | Vibration sampling and filtering baseline | Passed on hardware (2026-08-09) |
| 6 | Labeled threshold dataset collection | Passed on hardware (2026-08-09) |
| 7 | Hysteretic RUN/STOP state classifier | Passed on hardware (2026-08-10) |
| 8 | Mounted machine dataset | Waiting for supervised machine access |
| 9 | Production threshold calibration | Software complete; waiting for Stage 8 data |
| 10 | WiFi telemetry integration | Passed on connected bench hardware (2026-08-10) |
| 11 | Phone alerts | Passed with ServerChan/WeChat on hardware (2026-08-10) |
| 12 | Local interface and auxiliary sensors | MPU + dual LM393 + WiFi bench integration passed; site lamp calibration pending |
| 13 | Enclosure and soak validation | Analyzer complete; waiting for hardware soak |

Progress only advances after the current stage is verified on the real hardware.

See [docs/verification-log.md](docs/verification-log.md) for the exact tool versions and captured hardware results.

## Stage 0/1 quick start

1. Follow [docs/stage-0-1.md](docs/stage-0-1.md).
2. Open `firmware/stage_01_serial_test/stage_01_serial_test.ino` in Arduino IDE.
3. Select `ESP32 Dev Module` and `COM6`.
4. Compile, upload, then open Serial Monitor at `115200 baud`.
5. Report the compile/upload result and the first several Serial Monitor lines before proceeding.

Stage 2 instructions are in [docs/stage-2-wifi.md](docs/stage-2-wifi.md). Do not start Stage 3 wiring until Stage 2 is verified.

## Repository layout

```text
firmware/
  stage_01_serial_test/
    stage_01_serial_test.ino
  stage_02_wifi_test/
    secrets.example.h
    stage_02_wifi_test.ino
  stage_03_i2c_scanner/
    stage_03_i2c_scanner.ino
  stage_04_accel_read/
    stage_04_accel_read.ino
  stage_05_vibration_sampling/
    stage_05_vibration_sampling.ino
  stage_06_threshold_dataset/
    stage_06_threshold_dataset.ino
  stage_07_state_classifier/
    stage_07_state_classifier.ino
  stage_10_wifi_telemetry/
    secrets.example.h
    stage_10_wifi_telemetry.ino
docs/
  stage-0-1.md
  stage-2-wifi.md
  stage-3-i2c.md
  stage-4-acceleration.md
  stage-5-vibration-sampling.md
  stage-6-threshold-dataset.md
  stage-7-state-classifier.md
  stage-8-mounted-dataset.md
  stage-9-production-thresholds.md
  stage-10-wifi-telemetry.md
  stage-11-phone-alerts.md
  stage-12-local-interface.md
  stage-13-enclosure-soak.md
  troubleshooting.md
tools/
  analyze_threshold_dataset.ps1
  analyze_soak_log.ps1
```

## Planned hardware

- ESP32 development board with ESP-32E module and CH340 USB-to-serial bridge
- GY-521 module with an MPU6050-compatible, non-standard-identity sensor (not connected in Stage 0/1)
- Breadboard and jumper wires (not needed in Stage 0/1)
- Active buzzer module (later stage)
- Two LM393 light sensor modules (later stage)
- Optional SSD1306 OLED (later stage)
