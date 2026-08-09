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

## Current status

Waiting for the user to provide and photograph the exact modules before wiring. No speculative pinout or powered connection is authorized.
