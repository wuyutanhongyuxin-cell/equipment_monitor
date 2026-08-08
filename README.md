# ESP32 Industrial Equipment Monitor

Low-voltage, external monitor for industrial equipment. The planned MVP uses an ESP32 and an MPU6050 to detect machine vibration, classify RUN/STOP state, and send phone alerts without modifying the monitored machine.

## Safety boundary

- This project is external and low voltage only.
- Do not connect the monitor to 220 V/380 V power, internal machine terminals, spindle-drive power, or servo-drive power.
- Stage 0/1 uses only the ESP32 and its USB connection. Do not connect the MPU6050 yet.

## Current progress

| Stage | Scope | Status |
|---|---|---|
| 0 | Arduino IDE and ESP32 board support | Waiting for hardware verification |
| 1 | Compile, upload, and serial output | Waiting for hardware verification |
| 2-13 | WiFi through enclosure integration | Not started |

Progress only advances after the current stage is verified on the real hardware.

## Stage 0/1 quick start

1. Follow [docs/stage-0-1.md](docs/stage-0-1.md).
2. Open `firmware/stage_01_serial_test/stage_01_serial_test.ino` in Arduino IDE.
3. Select `ESP32 Dev Module` and `COM6`.
4. Compile, upload, then open Serial Monitor at `115200 baud`.
5. Report the compile/upload result and the first several Serial Monitor lines before proceeding.

## Repository layout

```text
firmware/
  stage_01_serial_test/
    stage_01_serial_test.ino
docs/
  stage-0-1.md
  troubleshooting.md
```

## Planned hardware

- ESP32 development board with ESP-32E module and CH340 USB-to-serial bridge
- GY-521 MPU6050 module (not connected in Stage 0/1)
- Breadboard and jumper wires (not needed in Stage 0/1)
- Active buzzer module (later stage)
- Two LM393 light sensor modules (later stage)
- Optional SSD1306 OLED (later stage)

