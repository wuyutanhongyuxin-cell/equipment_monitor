# Stage 12: Integrated vibration, dual-light, and WiFi monitor

## Goal

Verify that the MPU6050 200 Hz vibration path, two LM393 ADC1 inputs, and non-blocking WiFi status endpoint can run together without sample failures, missed periods, resets, or channel loss.

## Pin map

| Function | ESP32 pin |
|---|---|
| MPU6050 SDA | GPIO21 |
| MPU6050 SCL | GPIO22 |
| LM393 S1 AO | GPIO34 / ADC1 |
| LM393 S2 AO | GPIO35 / ADC1 |
| All module power | 3.3 V only |
| All module ground | Common GND |

Both LM393 DO pins remain disconnected. The light inputs are sampled once per completed vibration window and are not read inside the 200 Hz MPU6050 loop.

## WiFi behavior

The local ignored secrets file contains independently named credentials for `DAVOSEMI` and the earlier development network. The firmware rotates through configured networks after unsuccessful 10-second connection attempts. SSIDs may be logged; passwords are never logged or committed.

## Connected result

Passed on connected bench hardware on 2026-08-11 using the factory `DAVOSEMI` 2.4 GHz network. The ESP32 received `192.168.6.116` during this test. A real `/status` response included vibration state, both light channels, sensor health, RSSI, uptime, and explicit `light_calibrated=false` and `production_ready=false` flags.

A 60-request stability run completed with:

| Check | Result |
|---|---:|
| HTTP requests | 60/60 successful |
| Snapshot sequence | 585 to 653 |
| Maximum MPU sample failures | 0 |
| Maximum missed periods | 0 |
| Maximum snapshot age | 1002 ms |
| S1 observed range | 491-848 |
| S2 observed range | 578-1335 |
| WiFi RSSI | -55 to -48 dBm |

The connected bench integration passed. The DHCP address is not a permanent device address. Site mounting, independent RUN/ALARM lamp calibration, sensor shrouding, and state-fusion rules remain pending.
