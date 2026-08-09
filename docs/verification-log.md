# Hardware verification log

## Stage 0 and Stage 1

Verified on 2026-08-08 using the connected Windows computer and physical ESP32 board.

### Environment

- Arduino IDE: 2.3.9
- Arduino ESP32 Core: `esp32:esp32@3.3.10-cn`
- Board target: `ESP32 Dev Module` (`esp32:esp32:esp32`)
- Serial port: `COM6`, USB-SERIAL CH340
- Serial baud rate: 115200

The Espressif official China mirror was used because GitHub-hosted toolchain downloads timed out on the local network.

### Detected hardware

```text
Chip type: ESP32-D0WD-V3 (revision v3.1)
Features: Wi-Fi, BT, Dual Core + LP Core, 240MHz
Crystal frequency: 40MHz
```

The generic `ESP32 Dev Module` target compiled and uploaded successfully. No BOOT-button intervention was required.

### Compile result

```text
Sketch uses 281428 bytes (21%) of program storage space.
Global variables use 22092 bytes (6%) of dynamic memory.
```

### Upload result

All bootloader, partition, boot-app, and application writes completed. Each written section passed hash verification, followed by an automatic hard reset. COM6 remained available.

### Serial result

Captured after a normal reset at 115200 baud:

```text
ESP32 monitor boot OK
counter: 1
counter: 2
counter: 3
counter: 4
counter: 5
counter: 6
```

### Conclusion

Stage 0 and Stage 1 pass. Do not connect the MPU6050 until Stage 2 WiFi work is explicitly started.

### Arduino IDE startup correction

On 2026-08-09, the IDE was observed remaining on its startup logo. Captured startup logs showed that its local gRPC connection to `arduino-cli daemon` was reset. The Windows user had `HTTP_PROXY` and `HTTPS_PROXY` pointing to `127.0.0.1:10811`, but no localhost proxy bypass.

The user-level environment was corrected with:

```text
NO_PROXY=127.0.0.1,localhost
```

After a clean restart, the IDE reached the `ready` application state, opened `stage_01_serial_test`, and detected COM6. The existing external proxy settings were retained.

## Stage 2

Verified on 2026-08-09 using an iPhone Personal Hotspot with Maximize Compatibility enabled.

### Network

- SSID: `iPhone`
- Band: 2.4 GHz
- Channel observed by Windows: 6
- Security: WPA2-Personal
- Credentials: stored only in the Git-ignored local `secrets.h`

### Compile and upload

```text
Sketch uses 888312 bytes (67%) of program storage space.
Global variables use 45264 bytes (13%) of dynamic memory.
```

The sketch compiled for `ESP32 Dev Module`, uploaded through COM6, and all written sections passed hash verification. No BOOT-button intervention was required.

### Connection and recovery

The initial connection attempts reported `WL_DISCONNECTED` while the hotspot was not ready. The program continued running and retrying every ten seconds instead of blocking. Once the hotspot was available, the same running firmware recovered automatically without an ESP32 reset.

Captured serial output at 115200 baud:

```text
WiFi: connected, IP=172.20.10.2, RSSI=-12 dBm
WiFi: connected, IP=172.20.10.2, RSSI=-11 dBm
WiFi: connected, IP=172.20.10.2, RSSI=-10 dBm
WiFi: connected, IP=172.20.10.2, RSSI=-10 dBm
```

### Conclusion

Stage 2 passes. The ESP32 connects to 2.4 GHz WiFi, obtains a DHCP address, reports RSSI, and recovers after an unavailable-network interval without blocking the main loop. Do not wire the MPU6050 until Stage 3 instructions and voltage/pin checks are complete.

## Stage 3

First hardware attempt on 2026-08-09 after the MPU6050 wiring was reported connected.

### Compile and upload

The Stage 3 I2C scanner compiled successfully for `ESP32 Dev Module` using `esp32:esp32@3.3.10-cn`.

The sketch uploaded through `COM6`. The ESP32 was detected as:

```text
Chip type: ESP32-D0WD-V3 (revision v3.1)
Crystal frequency: 40MHz
```

All flashed sections passed hash verification and the board hard-reset through RTS.

### Initial scanner result

The first scanner firmware ran, but repeated scans found no I2C devices:

```text
I2C scan: starting
I2C scan: no devices found
I2C scan: starting
I2C scan: no devices found
I2C scan: starting
I2C scan: no devices found
```

### Diagnostic scanner result

The scanner was upgraded to test both the required primary mapping and a software-swapped SDA/SCL mapping. This checks whether the physical SDA/SCL wires are crossed without changing the wiring.

Captured serial output at 115200 baud:

```text
I2C mapping: primary, SDA=GPIO21, SCL=GPIO22
I2C scan: starting
I2C scan: no devices found
I2C mapping: swapped, SDA=GPIO22, SCL=GPIO21
I2C scan: starting
I2C scan: no devices found
I2C diagnostic: no devices on either mapping
I2C diagnostic: check 3V3, GND, breadboard rows, and jumper contact
```

The wiring was then photographed with both the ESP32 power LED and GY-521 LED lit. A retest under that visible powered condition still produced `no devices on either mapping`. This confirms that module LEDs are not sufficient Stage 3 evidence; the data lines still did not acknowledge on GPIO21/GPIO22 or the swapped GPIO22/GPIO21 test.

After the wiring was rechecked and reported correct, the scanner was extended with a wire-finder diagnostic that probes common ESP32 GPIO pin pairs for only the expected MPU6050 addresses. This also failed to find the sensor:

```text
I2C wire finder: probing 19 candidate GPIO pins for 0x68/0x69
I2C wire finder: no expected address found on candidate GPIO pairs
```

The GY-521 connector was reseated and the same diagnostic was run again. The primary GPIO21/GPIO22 scan, swapped GPIO22/GPIO21 scan, and 19-pin wire-finder scan still found no expected MPU6050 address.

The four connections were then remade with replacement jumper wires and retested. The result was unchanged: the primary GPIO21/GPIO22 scan, swapped GPIO22/GPIO21 scan, and 19-pin wire-finder scan all still failed to find `0x68` or `0x69`.

Because no replacement MPU6050 module was available, a line-level diagnostic was added. It showed GPIO21 behaving like it had an external pull-up, while GPIO22 did not:

```text
I2C line level: GPIO21, internal pulldown=HIGH, internal pullup=HIGH -> external high/pullup detected
I2C line level: GPIO22, internal pulldown=LOW, internal pullup=HIGH -> no external pullup detected
```

During the same diagnostic run, the sensor did briefly acknowledge at the expected address and the primary mapping produced one pass candidate:

```text
I2C mapping: primary, SDA=GPIO21, SCL=GPIO22
I2C scan: starting
I2C device found at 0x68
I2C scan: complete, devices=1
I2C diagnostic: primary mapping has exactly one expected MPU6050 address
I2C diagnostic: Stage 3 pass candidate on primary wiring
```

Later scans returned to `no devices found`, so the Stage 3 pass criteria were not met.

After another `P22 -> SCL` reseat, a longer serial capture showed major improvement. The primary GPIO21/GPIO22 mapping found exactly one expected device at `0x68` multiple times, including a run of more than three consecutive successful scans:

```text
I2C mapping: primary, SDA=GPIO21, SCL=GPIO22
I2C scan: starting
I2C device found at 0x68
I2C scan: complete, devices=1
I2C diagnostic: primary mapping has exactly one expected MPU6050 address
I2C diagnostic: Stage 3 pass candidate on primary wiring
```

However, the same capture later returned to `no devices found`, and the line-level diagnostic again showed GPIO22 without an external pull-up:

```text
I2C line level: GPIO21, internal pulldown=HIGH, internal pullup=HIGH -> external high/pullup detected
I2C line level: GPIO22, internal pulldown=LOW, internal pullup=HIGH -> no external pullup detected
```

A later 80-second capture improved further: the primary GPIO21/GPIO22 mapping produced a long burst of consecutive `0x68` acknowledgements, then dropped back to `no devices found` near the end of the same capture. During one dropout both GPIO21 and GPIO22 showed external pull-ups; during the next dropout GPIO22 again lost the external pull-up indication. This confirms the sensor and address are correct, but the connection is still intermittent over a longer observation window.

After rewiring with male-to-female jumpers, an 80-second capture found no `0x68` acknowledgements. The line-level diagnostic was consistent across repeated scans: GPIO21 reported an external pull-up, while GPIO22 reported no external pull-up. The failure therefore remains focused on the `P22 -> SCL` electrical path, not on power, WiFi, or the MPU6050 address.

After the wiring error was corrected, an 80-second capture showed stable Stage 3 behavior. Every scan in the capture used the primary mapping and found exactly one expected MPU6050 device at `0x68`:

```text
I2C mapping: primary, SDA=GPIO21, SCL=GPIO22
I2C scan: starting
I2C device found at 0x68
I2C scan: complete, devices=1
I2C diagnostic: primary mapping has exactly one expected MPU6050 address
I2C diagnostic: Stage 3 pass candidate on primary wiring
```

No `no devices found` result appeared during the stable capture.

### Conclusion

Stage 3 passes. The MPU6050 acknowledges at `0x68` on the required primary mapping, `SDA=GPIO21` and `SCL=GPIO22`, and the final 80-second capture remained stable. The earlier failures were caused by incorrect or unstable wiring on the `P22 -> SCL` path. Stage 4 may now install an MPU6050 library and read acceleration values.

## Stage 4

Verified on 2026-08-09 with the Stage 3 wiring left unchanged.

### Implementation

The Stage 4 sketch uses the ESP32 core `Wire` library at 100 kHz. It reads `WHO_AM_I`, wakes the MPU6050 through `PWR_MGMT_1`, and reads the six acceleration bytes beginning at `ACCEL_XOUT_H`. The default +/-2g scale converts raw values using 16384 LSB/g.

The sketch compiled for `ESP32 Dev Module`, uploaded through `COM6`, and all flashed sections passed hash verification.

### Acceleration result

A continuous capture of approximately 40 seconds produced more than 80 samples without any `MPU6050 read: failed` output. Representative stationary samples were:

```text
accel_g: x=-0.130, y=-0.017, z=+1.023, magnitude=1.032
accel_g: x=-0.126, y=-0.020, z=+1.029, magnitude=1.037
accel_g: x=-0.128, y=-0.019, z=+1.020, magnitude=1.028
accel_g: x=-0.130, y=-0.017, z=+1.033, magnitude=1.042
accel_g: x=-0.126, y=-0.024, z=+1.014, magnitude=1.022
```

The measured magnitude remained approximately 1.022g to 1.042g, inside the Stage 4 accepted stationary range of 0.8g to 1.2g.

### Conclusion

Stage 4 passes for acceleration behavior. The sensor provides stable, plausible three-axis data with no read failures during the hardware capture. Later Stage 5 diagnostics established that this module returns non-standard `WHO_AM_I=0x74` rather than the official MPU-6050 value `0x68`; therefore the fitted sensor is treated as MPU6050-compatible, not authenticated as an original TDK/InvenSense part. The Stage 4 sketch was corrected to halt on an unknown identity instead of returning from `setup()` and unintentionally continuing into `loop()`.

## Stage 5

Verified on 2026-08-09 using the unchanged breadboard wiring and the MPU6050-compatible sensor.

### Sampling design

- Accelerometer sample target: 200 Hz
- I2C clock: 50 kHz
- MPU6050-compatible DLPF setting: approximately 44 Hz
- Accelerometer range: +/-2g
- Startup gravity calibration: 400 valid samples
- Gravity/orientation tracking: 0.5 Hz low-pass
- Statistics window: 200 attempts / approximately 1 second

Each sample subtracts the slowly tracked three-axis gravity vector. The firmware calculates the RMS and peak magnitude of the remaining vibration vector for each window.

### Identity investigation

Initial Stage 5 attempts halted because the identity register consistently returned `0x74`. The Stage 3 scanner was reflashed as a differential check and found `0x68` on five consecutive address scans, proving that the I2C address path remained connected. Reflashing Stage 4 under the same reset conditions also read `WHO_AM_I=0x74` while continuing to produce plausible acceleration data.

The official MPU-6050 register map specifies `WHO_AM_I=0x68`; `0x74` is non-standard. Tests at both 100 kHz and 50 kHz, with repeated-start and stop-separated register reads, returned the same `0x74`. The hardware is therefore recorded as a compatible, non-authenticated sensor. Stage 5 explicitly reports this identity and proceeds only after the DLPF, sample-divider, and accelerometer-range registers are written and read back correctly.

### Hardware result

Initialization and calibration completed without I2C failures:

```text
MPU6050 init: WHO_AM_I=0x74, non-standard compatible device
MPU6050 init: ready, sample_target_hz=200, dlpf_hz=44, accel_range=+/-2g
Calibration: keep sensor stationary, samples=400
Calibration: ready, gravity_g=(-0.1259,-0.0202,+1.0229), failures=0
```

The stationary capture produced 42 consecutive complete windows. Every window reported `valid=200/200`, `failed=0`, and `missed=0`. The measured ranges were:

- Actual sample rate: 199.63 Hz to 200.00 Hz
- Stationary vibration RMS: 0.00416g to 0.00477g
- Stationary peak: 0.00817g to 0.01375g

Representative output:

```text
window: rate_hz=199.63, valid=200/200, failed=0, missed=0, vibration_rms_g=0.00475, peak_g=0.01148
window: rate_hz=200.00, valid=200/200, failed=0, missed=0, vibration_rms_g=0.00442, peak_g=0.01044
window: rate_hz=200.00, valid=200/200, failed=0, missed=0, vibration_rms_g=0.00416, peak_g=0.00975
```

### Conclusion

Stage 5 passes. The system now has a verified 200 Hz sampling pipeline, gravity removal, one-second RMS/peak windows, zero observed read failures or missed periods, and a measured stationary noise baseline. Stage 6 may collect deliberately moved or vibrating data and design classification thresholds; no RUN/STOP threshold has been selected yet.
