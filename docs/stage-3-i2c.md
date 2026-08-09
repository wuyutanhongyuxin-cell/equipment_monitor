# Stage 3: MPU6050 I2C communication

## Verified hardware

Hardware photos were inspected before wiring:

- Controller: Ai-Thinker NodeMCU-32 V1.3 with ESP-32E module
- Sensor board: GY-521 MPU-6050 breakout
- Controller silkscreen exposes `3V3`, `GND`, `P21`, and `P22`
- GY-521 silkscreen order is `VCC`, `GND`, `SCL`, `SDA`, `XDA`, `XCL`, `AD0`, `INT`
- No obvious solder bridge was visible on the photographed GY-521 header

The local `photo/` directory is intentionally excluded from Git because hardware photos may contain personal or site details.

## Current goal

Verify electrical power and I2C acknowledgement only. Do not install an MPU6050 library and do not read acceleration yet.

## Power-off rule

Disconnect the ESP32 Micro USB cable before inserting, removing, or moving any jumper wire. Never move a wire while the board is powered.

## Wiring

Use four female-to-female jumper wires because both photographed boards have male header pins.

| ESP32 NodeMCU-32 silkscreen | GY-521 silkscreen | Purpose |
|---|---|---|
| `3V3` | `VCC` | 3.3 V sensor power |
| `GND` | `GND` | Common ground |
| `P21` | `SDA` | I2C data, GPIO21 |
| `P22` | `SCL` | I2C clock, GPIO22 |

Follow the printed labels, not wire color and not pin-counting alone. Leave `XDA`, `XCL`, `AD0`, and `INT` unconnected.

The GY-521 must be powered from `3V3` for this project. Do not connect its VCC to `5V`, and never connect 5 V to an ESP32 GPIO.

## Pre-power inspection

Before reconnecting USB, verify all four statements visually:

1. ESP32 `3V3` reaches only GY-521 `VCC`.
2. ESP32 `GND` reaches only GY-521 `GND`.
3. ESP32 `P21` reaches only GY-521 `SDA`.
4. ESP32 `P22` reaches only GY-521 `SCL`.

Take a clear photo showing both boards and all four wire endpoints before applying power.

## Scanner behavior

The scanner uses the ESP32 Arduino `Wire` library built into the board core:

- SDA: GPIO21
- SCL: GPIO22
- Clock: 100 kHz for conservative bring-up
- Bus timeout: 50 ms
- Scan interval: 5 seconds
- Serial baud: 115200

No external sensor library is required for Stage 3.

## Expected output

With AD0 left unconnected on a normal GY-521, the expected address is usually `0x68`:

```text
ESP32 I2C scanner boot OK
I2C pins: SDA=GPIO21, SCL=GPIO22, frequency=100000 Hz
I2C scan: starting
I2C device found at 0x68
I2C scan: complete, devices=1
```

If AD0 is pulled high, `0x69` is valid instead. No other address is accepted as MPU6050 evidence without further investigation.

## Pass criteria

- No component becomes hot and no burning smell is present.
- The ESP32 remains available on COM6.
- At least three consecutive scans find exactly one expected device at `0x68` or `0x69`.

Only after these criteria pass may Stage 4 install an MPU6050 library and read acceleration.
