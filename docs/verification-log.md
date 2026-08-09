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
