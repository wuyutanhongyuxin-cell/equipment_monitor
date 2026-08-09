# Stage 2: WiFi connection test

## Current goal

Verify this path without connecting any sensor:

```text
ESP32 -> 2.4 GHz WiFi -> DHCP address -> periodic RSSI output
```

## Safety and scope

- Keep the ESP32 connected only by USB.
- Do not connect the MPU6050, buzzer, light sensors, or industrial equipment.
- This stage does not send phone notifications.

## Credentials

Create `firmware/stage_02_wifi_test/secrets.h` from `secrets.example.h`, then enter the WiFi SSID and password. The real `secrets.h` is excluded by `.gitignore` and must not be committed or shown in screenshots.

Use a 2.4 GHz WiFi network. The classic ESP32-D0WD-V3 detected in Stage 1 does not connect to a 5 GHz-only network. A router using the same SSID for 2.4 GHz and 5 GHz may work if 2.4 GHz is enabled.

## Behavior

- Serial baud rate: 115200.
- The program starts a connection attempt without an infinite blocking loop.
- It reports status every five seconds.
- While disconnected, it retries every ten seconds.
- Sensor work can continue in later stages because WiFi failure does not block `loop()`.

## Expected output

```text
ESP32 WiFi test boot OK
WiFi: connecting to example-network
WiFi: connection established
WiFi: connected, IP=192.168.1.123, RSSI=-52 dBm
```

The IP address and RSSI will differ. RSSI is normally negative; a value closer to zero indicates a stronger signal.

## Pass criteria

- The sketch compiles for `ESP32 Dev Module`.
- It uploads through COM6.
- It obtains a non-zero local IP address.
- It prints `WiFi: connected` repeatedly without restarting.
- Disconnecting WiFi does not freeze the program; it continues printing status and retrying.

Do not proceed to Stage 3 until the connected result is captured from the physical ESP32.
