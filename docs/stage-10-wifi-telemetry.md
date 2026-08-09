# Stage 10: WiFi telemetry integration

## Goal

Publish current state, vibration RMS, sensor health, sample quality, uptime, and transition time over WiFi without blocking the 200 Hz sampling loop.

## Required decisions

- Destination protocol and endpoint: local HTTP/MQTT server or an approved cloud service.
- Authentication and certificate requirements.
- Data retention and site-network policy.
- Offline queue size and retry policy.

## Pass criteria

- Sampling remains 200/200 with zero missed periods during connection loss and reconnect.
- Network operations never run in the sampling path.
- Credentials remain in ignored local secrets, never source control or serial logs.
- State eventually synchronizes after a network outage without duplicate transition events.

## Current status

Blocked on the Stage 9 production classifier and destination/service choice. Stage 2 already verifies non-blocking 2.4 GHz WiFi reconnect on this ESP32.
