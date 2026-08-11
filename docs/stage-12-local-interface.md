# Stage 12: Local interface and auxiliary sensors

## Goal

Add only the locally required status outputs and auxiliary inputs after the final pin map and hardware variants are verified.

## Planned hardware requiring inspection

- Active buzzer module
- Two LM393 light sensor modules
- Optional SSD1306 OLED

## Required decisions

- Exact module photos, voltage requirements, active-high/active-low behavior, and connector labels.
- Final pin allocation avoiding boot-strapping pins and the verified I2C bus.
- Whether the buzzer is required, allowed on site, and how it can be silenced/tested.
- Whether light sensors and OLED are necessary for the MVP.

## Pass criteria

- Each module is brought up separately at low voltage before integration.
- No GPIO receives 5 V.
- Startup, fault, RUN, STOP, and mute behavior are unambiguous.
- Added modules do not cause sensor read failures, missed samples, or boot problems.

## Hardware-independent policy

`firmware/common/local_indicator_policy.h` defines abstract output patterns before any GPIO polarity is assumed: UNKNOWN slowly blinks, STOP is off, RUN is solid, and sensor/offline faults fast-blink with a pulsed buzzer unless muted. Fault indication overrides RUN/STOP, while mute never hides the visual fault.

Run the host regression suite with:

```powershell
.\tests\test_local_indicator_policy.ps1
```

## Current status

Hardware-independent output policy implemented. Both LM393 modules passed separate and simultaneous 3.3 V tests, then passed integration with MPU6050 sampling and WiFi telemetry on 2026-08-11; see [stage-12-light-sensor.md](stage-12-light-sensor.md) and [stage-12-integrated-monitor.md](stage-12-integrated-monitor.md). Per-site RUN/ALARM lamp mounting and calibration remain pending. The buzzer and optional OLED remain optional and untested.
