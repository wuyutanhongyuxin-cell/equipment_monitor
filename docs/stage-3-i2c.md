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

Both photographed boards have male header pins, while the available jumper set does not include female-to-female wires. For the verified hardware set, plug both modules into the MB-102 breadboard and use four male-to-male jumper wires between breadboard tie points. Do not attempt to join two jumper wires end-to-end as a permanent connection.

Place the ESP32 so its two header rows straddle the breadboard center channel. Place the GY-521 in a separate clear area with each of its eight pins in a different numbered breadboard row. Do not put two GY-521 pins into the same connected five-hole row.

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

## Diagnostic scanner behavior

The scanner uses the ESP32 Arduino `Wire` library built into the board core:

- SDA: GPIO21
- SCL: GPIO22
- Clock: 100 kHz for conservative bring-up
- Bus timeout: 50 ms
- Scan interval: 5 seconds
- Serial baud: 115200

No external sensor library is required for Stage 3.

The scanner first tests the required primary mapping:

- SDA: GPIO21
- SCL: GPIO22

If the primary mapping does not find exactly one MPU6050 address, it then tests a software-swapped mapping:

- SDA: GPIO22
- SCL: GPIO21

The swapped scan is diagnostic only. A response only on the swapped mapping means the physical SDA/SCL wires are probably crossed; it is not a Stage 3 pass.

If neither primary nor swapped mapping finds a device, the scanner also probes a wider set of common ESP32 GPIO pairs for only the expected MPU6050 addresses, `0x68` and `0x69`. This wire-finder step helps detect a wire placed on the wrong ESP32 GPIO. A wire-finder hit is still not a pass; the wiring must be corrected back to GPIO21/GPIO22 and retested.

The scanner also prints a line-level diagnostic for GPIO21 and GPIO22. A healthy connected I2C idle line is expected to read HIGH. If a line prints `internal pulldown=LOW, internal pullup=HIGH`, the ESP32 is not seeing an external pull-up on that line; inspect that specific wire and the matching GY-521 header pin.

## Expected output

With AD0 left unconnected on a normal GY-521, the expected address is usually `0x68`:

```text
ESP32 I2C diagnostic scanner boot OK
I2C primary wiring: SDA=GPIO21, SCL=GPIO22, frequency=100000 Hz
I2C mapping: primary, SDA=GPIO21, SCL=GPIO22
I2C scan: starting
I2C device found at 0x68
I2C scan: complete, devices=1
I2C diagnostic: primary mapping has exactly one expected MPU6050 address
I2C diagnostic: Stage 3 pass candidate on primary wiring
```

If AD0 is pulled high, `0x69` is valid instead. No other address is accepted as MPU6050 evidence without further investigation.

If the scanner prints `no devices on either mapping` and `wire finder: no expected address found`, do not proceed to Stage 4. Power off the ESP32 and inspect:

- module LEDs only prove that power is present; they do not prove SDA/SCL communication
- whether GY-521 `VCC` is really connected to ESP32 `3V3`
- whether GY-521 `GND` shares ESP32 `GND`
- whether each GY-521 pin is in a separate breadboard row
- whether the jumper ends actually contact the same breadboard tie points as the module pins
- whether the ESP32 is straddling the breadboard center channel instead of shorting both sides into one row group
- whether the GY-521 `SCL` and `SDA` header solder joints are actually connected, not just mechanically present

If GPIO21 reports external pull-up but GPIO22 reports no external pull-up, focus on `P22 -> SCL`. If GPIO22 reports external pull-up but GPIO21 does not, focus on `P21 -> SDA`.

## Pass criteria

- No component becomes hot and no burning smell is present.
- The ESP32 remains available on COM6.
- At least three consecutive scans find exactly one expected device at `0x68` or `0x69`.

Only after these criteria pass may Stage 4 install an MPU6050 library and read acceleration.
