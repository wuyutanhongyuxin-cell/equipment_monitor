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

## Stage 6

Started on 2026-08-09. The labeled collection firmware compiled, uploaded to `COM6`, verified its configuration registers, calibrated with 400 valid samples and zero failures, and waited for an explicit serial `g` command before each run.

Two collection attempts each produced 15 stationary and 15 nominally vibrating windows. Every window contained `valid=200/200`, `failed=0`, and `missed=0`, at 199.63 Hz to 200.00 Hz. The first attempt contained only one clearly disturbed vibration window (`rms_g=0.07567`, `peak_g=0.62368`); the other 14 vibration windows remained near the stationary baseline. The second attempt contained no sustained vibration response, and one stationary window was disturbed instead.

Both attempts correctly reported:

```text
threshold_candidate: none, datasets overlap
```

### Current conclusion

Stage 6 is not yet passed. The acquisition pipeline and rejection logic work, but the deliberate physical vibration was not sustained within the labeled LED-on interval. A new run must be explicitly coordinated with the operator. No threshold is accepted from these overlapping datasets.

### Split-phase synchronization update

The firmware was changed so stationary collection starts only with `s`, then pauses indefinitely before vibration collection starts with `v`. This removed the automatic countdown race and produced clean stationary datasets with RMS ranges of 0.00423g to 0.00472g and 0.00417g to 0.00467g.

Two split-phase vibration attempts still started too late relative to the serial command. The first contained no physical response. In the second, windows 12 through 15 increased from `rms_g=0.03018` to `0.21148`, while windows 1 through 11 remained near 0.004g. This confirms that deliberate movement is strongly detectable, but user-message latency still contaminated the nominal vibration label. The next attempt must begin physical movement before the `v` command is sent and continue for at least 20 seconds.

### Final synchronized dataset

For the final attempt, physical movement began before the `v` command and continued throughout the complete 15-second vibration capture. All 30 labeled windows reported 200 valid samples, zero I2C failures, zero missed sample periods, and a measured rate of 199.63 Hz to 200.00 Hz.

```text
summary,label=STILL,windows=15,rms_min=0.00437,rms_mean=0.00738,rms_max=0.01909,peak_max=0.05835
summary,label=VIBRATION,windows=15,rms_min=0.21883,rms_mean=0.32073,rms_max=0.45213,peak_max=2.69212
threshold_candidate: vibration_rms_g=0.11896, separation_gap_g=0.19974
```

The stationary and vibration RMS ranges are completely separated. The candidate `0.11896g` threshold is the midpoint between the observed stationary maximum and vibration minimum.

### Final conclusion

Stage 6 passes. The labeled acquisition and overlap-rejection workflow is verified, and deliberate bench movement is separable from stationary noise in this dataset. The `0.11896g` value is a bench-test candidate only; it must not be treated as a real equipment RUN/STOP threshold until the sensor is mounted in its intended location and actual machine-state datasets are collected.

## Stage 7

Started on 2026-08-09. The classifier uses bench-only thresholds of 0.120g to enter RUN and 0.060g to enter STOP, with two consecutive valid one-second windows required for either transition.

### Timing defect and correction

The first hardware run alternated between a valid 200-sample window and an invalid 199-sample window. Sensor reads themselves had zero failures. Diagnosis showed that two serial log lines blocked longer than one 5 ms sample period, while the next window retained a timestamp captured before logging.

The firmware now emits shorter logs and realigns the next sample deadline only after logging completes. A temporary 230400-baud test removed missed windows but produced unreliable CH340 text, so the final firmware returned to 115200 baud while retaining post-log realignment. The final stationary capture contained consecutive 200/200 windows with zero failures and zero missed periods at 199.63 Hz.

### Automatic and stationary result

The startup synthetic self-test verified all classifier rules and reported `PASS`:

- two low windows: `UNKNOWN -> STOP`
- one high window: remain `STOP`
- hysteresis-band window: remain `STOP` and clear pending evidence
- two high windows: `STOP -> RUN`
- one low window: remain `RUN`
- two low windows: `RUN -> STOP`

After self-test reset and real sensor calibration, stationary hardware data produced:

```text
state=UNKNOWN,rms=0.00440,peak=0.01081,evidence=STOP,confirm=1/2,valid=200/200,failed=0,missed=0
transition: from=UNKNOWN,to=STOP,rms_g=0.00432
state=STOP,rms=0.00432,peak=0.00924,evidence=STOP,confirm=2/2,valid=200/200,failed=0,missed=0
```

### Current conclusion

Stage 7 was initially partially verified: compilation, upload, deterministic state-machine self-test, real stationary `UNKNOWN -> STOP`, and continuous sampling passed. A final real-sensor movement sequence was reserved for operator-assisted validation.

### Final real-sensor transition validation

Completed on 2026-08-10. Before movement, five consecutive stationary windows remained in `STOP` with RMS from 0.00437g to 0.00478g. During sustained movement, the classifier reached and held `RUN`; seven captured RUN windows had RMS from 0.38305g to 0.46532g. After the board was placed back on the table, six captured windows reached and held `STOP` with RMS from 0.00414g to 0.00461g.

Every captured transition-validation window contained 200/200 samples, zero read failures, zero missed periods, and an actual window sample rate of 199.63 Hz. The deterministic boot self-test separately proves that each transition requires two consecutive supporting windows and that a single opposite-state window or a hysteresis-band window cannot switch state.

### Final conclusion

Stage 7 passes. The final firmware verifies its state logic at boot and has completed real-sensor `UNKNOWN -> STOP`, `STOP -> RUN`, and `RUN -> STOP` behavior while retaining valid 200 Hz acquisition. The thresholds remain bench-only and must be replaced or validated using Stage 8 mounted-machine data.

## Stages 8-13 preparation

The remaining stages were reviewed and scoped without claiming hardware completion:

- Stage 8: supervised mounted-machine STOP/RUN dataset; blocked on safe site access and final external mount.
- Stage 9: production threshold calibration; analyzer implemented and tested for separated, overlapping, and insufficient datasets; blocked on Stage 8 data.
- Stage 10: non-blocking WiFi telemetry; blocked on production classifier and endpoint/protocol decision.
- Stage 11: phone alert deduplication and policy; blocked on transport, provider, recipient, and alert policy.
- Stage 12: buzzer, light sensors, and optional OLED; blocked on exact module inspection and final pin allocation.
- Stage 13: enclosure and soak validation; blocked on prior stages, enclosure, and supervised installation.

No external notification account, network endpoint, machine connection, or speculative hardware wiring was created during this preparation.

## Device-independent and no-factory-equipment work

Completed on 2026-08-10 after Stage 7 hardware validation.

### Stage 9 software

The production-threshold analyzer now has a repeatable PowerShell regression suite. It verifies separated ranges and their midpoint, rejects overlapping ranges without producing a threshold, and rejects datasets below the required per-class window count.

```text
Stage 9 threshold analyzer tests: PASS
```

Stage 9 software is complete, but production thresholds remain blocked on supervised Stage 8 mounted-machine data.

### Stage 10 bench telemetry

A local read-only HTTP `/status` implementation was added. It reuses the Stage 7 classifier through optional hooks, while WiFi connection and HTTP clients run in a FreeRTOS task pinned to core 0. The response explicitly reports `threshold_source=bench_stage6` and `production_ready=false`.

The firmware compiled and uploaded through `COM6`. With the saved 2.4 GHz hotspot unavailable, repeated reconnect attempts ran while more than 30 consecutive sensor windows remained 200/200 with zero failures and zero missed periods at 199.63 Hz. This verifies the disconnected/non-blocking path.

Connected validation then used a 2.4 GHz network. The board obtained `192.168.101.19`, and a real `/status` request returned `sensor_healthy=true`, current STOP state, zero failed samples, and zero missed periods. A 60-second stability run completed 60/60 HTTP requests; sequence advanced from 753 to 813, maximum data age was 1000 ms, failed and missed counters remained zero, and RSSI ranged from -73 to -66 dBm.

A compile-time, default-disabled self-test forced one disconnect at 15.37 seconds. The HTTP service was announced again at 16.66 seconds, approximately 1.29 seconds later. Every observed window before, during, and after reconnect remained 200/200 with zero failures and zero missed periods at approximately 199.63 Hz. The default firmware with the self-test disabled was restored after this test.

### Stage 11 software core

The provider-independent fixed-capacity alert policy implements silent initial state, duplicate-state suppression, boot ID plus monotonic sequence, head-matching delivery acknowledgement, minimum send interval, offline queueing, oldest-event overflow, and an observable dropped-event counter.

```text
Stage 11 alert policy tests: PASS
```

Real phone delivery remains pending because no provider, recipient, credentials, or alert policy has been authorized.

The ServerChan gateway was then implemented for a personal WeChat delivery path. Endpoint selection supports both `SCT` Turbo and `sctp` ServerChan 3 keys, credentials load from an ignored local file or process environment, and HTTP delivery uses the operating system HTTPS validation. Host endpoint, transition, deduplication, offline/recovery queue, and message-format tests pass. An eight-second dry-run consumed the live Stage 10 endpoint without sending an external message. Live receipt remains pending SendKey configuration.

```text
Stage 11 phone alert gateway tests: PASS
```

A ServerChan Turbo key was configured in the ignored local secrets file. The provider accepted a minimal delivery test with `code=0`, and the user confirmed receipt in personal WeChat. A first live hand-motion run then delivered noisy RUN/STOP/RUN transitions five seconds apart, so the gateway was stopped and a second alert-layer confirmation rule was added.

Final connected validation completed on 2026-08-10. Windows required the local status request to use the configured HTTP proxy, while ServerChan HTTPS succeeded only through the direct Windows TLS path. The gateway now separates these routes and emits credential-free observation and delivery diagnostics. A short movement was suppressed without a notification. Sustained movement was observed as RUN and produced one confirmed `[RUNNING]` WeChat notification. After the sensor returned to a stable STOP, one `[STOPPED]` notification arrived after the configured 60-second minimum send interval. The user confirmed both messages, and the gateway was stopped after the test. Stage 11 passed.

### Stage 12 software policy

The hardware-independent local output policy maps UNKNOWN, STOP, RUN, sensor fault, offline fault, and mute to abstract LED/buzzer patterns. Faults override state; mute suppresses only sound and never hides visual fault status.

```text
Stage 12 local indicator policy tests: PASS
```

GPIO binding and powered tests remain pending exact module inspection.

### Stage 12 first LM393 analog sensor

The photographed module exposes labeled `AO`, `DO`, `GND`, and `VCC` pins. One module was powered from 3.3 V and its AO pin was connected to ADC1 GPIO34; DO remained disconnected. The isolated sketch compiled with ESP32 Core 3.3.10-cn, uploaded through COM6, and produced stable readings without resets or USB faults.

Twenty-six samples per condition measured raw ADC values of 587-591 in room light, 3555-3607 when fully covered, and 0 under a phone flashlight at approximately 10 cm. This verifies the module's inverse response and clear separation across the tested range. The calibrated millivolt helper reported a non-zero floor at raw zero, so later light classification will use raw ADC values as its primary input. The first module passed.

The second module then passed the same isolated GPIO34 test. It measured raw values of 637-640 in room light, 3113-3156 when fully covered, and 0 under the same approximate flashlight condition. Both modules share the same response direction but have measurably different baselines and covered values, confirming that production calibration must be per channel. Simultaneous two-channel integration remains pending.

On 2026-08-11 both modules were powered together from 3.3 V, with S1 AO on ADC1 GPIO34 and S2 AO on ADC1 GPIO35. Room-light means were 530 and 463. Covering only S1 produced means of 3141 and 705; covering only S2 produced means of 404 and 3942. Covering both produced means of 3430 and 4083. Both channels therefore respond independently and can reach their dark ranges together without reset or supply instability. The dual-input bench test passed; main-firmware integration and per-site lamp calibration remain pending.

The MPU6050 and both LM393 modules were then connected together. The integrated firmware retained 200/200 MPU samples at approximately 199.63 Hz with zero I2C failures and zero missed periods while sampling both ADC1 channels once per one-second vibration window. The factory `DAVOSEMI` SSID was confirmed as 2.4 GHz and stored only in the ignored secrets file alongside the earlier development credentials. The network task connected on its first attempt and received `192.168.6.116` for this DHCP session.

The integrated `/status` endpoint returned both light channels together with vibration and health telemetry. A 60-second run completed 60/60 requests; sequence advanced from 585 to 653, maximum sample failures and missed periods remained zero, maximum snapshot age was 1002 ms, S1 ranged from 491 to 848, S2 ranged from 578 to 1335, and RSSI ranged from -55 to -48 dBm. Bench integration passed. Site lamp mounting, optical isolation, per-channel calibration, and fusion rules remain pending.

### Stage 13 software analyzer

The soak analyzer checks minimum complete-window count, incomplete windows, failed samples, missed periods, state counts, and RMS range. Its production default requires 86,400 complete one-second windows.

```text
Stage 13 soak analyzer tests: PASS
```

The physical 24-hour soak and mounted validation remain pending the enclosure and completed upstream stages.
