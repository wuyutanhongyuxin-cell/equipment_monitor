# Stage 10: WiFi telemetry integration

## Goal

Publish current state, vibration RMS, sensor health, sample quality, uptime, and transition time over WiFi without blocking the 200 Hz sampling loop.

## Bench implementation

The first implementation exposes a read-only local HTTP endpoint at `/status`. WiFi and client handling run in a FreeRTOS task pinned to core 0; the 200 Hz sensor/classifier loop remains on the Arduino loop core. The response includes state, RMS, peak, sequence, sample failures, missed periods, data age, sensor health, RSSI, uptime, threshold source, and an explicit `production_ready=false` flag.

Credentials load from ignored `secrets.h`. For local development, the existing ignored Stage 2 credentials can also be reused without copying their contents.

`WIFI_RECONNECT_SELF_TEST_MS` is a compile-time diagnostic switch. It defaults to zero and therefore has no production behavior. Setting it to a positive millisecond value forces one WiFi disconnect after boot so reconnect handling can be verified without interrupting the access point.

## Pass criteria

- Sampling remains 200/200 with zero missed periods during connection loss and reconnect.
- Network operations never run in the sampling path.
- Credentials remain in ignored local secrets, never source control or serial logs.
- State eventually synchronizes after a network outage without duplicate transition events.

## Remaining production decisions

- Whether local HTTP is sufficient or an authenticated MQTT/HTTPS destination is required.
- Authentication, TLS, network segmentation, and site access policy.
- Data retention and offline queue requirements.

## Current status

Connected bench validation passed on 2026-08-10. Sixty consecutive `/status` requests succeeded, snapshot sequence advanced, and sample failures and missed periods remained zero. A controlled disconnect recovered the HTTP service in approximately 1.29 seconds while every observed sensor window remained 200/200 at approximately 199.63 Hz with zero failures and zero missed periods.

The board was restored to a build with the reconnect self-test disabled after validation. Production release remains blocked on the Stage 9 production classifier and site network/security decisions.
