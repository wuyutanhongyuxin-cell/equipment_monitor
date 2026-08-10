# Stage 11: Phone alerts

## Goal

Send actionable phone notifications for confirmed state transitions and sensor/offline faults, with deduplication and rate limiting.

## Required decisions

- Approved notification provider and recipient account/device.
- Which transitions alert, allowed delay, quiet hours, and escalation rules.
- Offline/fault timeout and recovery notification behavior.

## Provider-independent core

`firmware/common/alert_policy.h` implements fixed-capacity transition queuing without dynamic allocation. Initial state observation is silent, repeated identical states are deduplicated, queued events retain a boot ID and monotonic sequence for idempotency, delivery acknowledgement must match the queue head, minimum send intervals are enforced, and overflow drops the oldest event while incrementing an observable counter.

Run the host regression suite with:

```powershell
.\tests\test_alert_policy.ps1
```

## ServerChan gateway

`tools/run_phone_alert_gateway.ps1` polls the local Stage 10 `/status` endpoint and sends queued transitions through ServerChan over HTTPS. The first observation is silent. A changed device state must remain stable for 10 seconds before it is queued, repeated states are deduplicated, sensor faults, offline state, and recovery are represented as transitions, failed deliveries remain queued for retry, and a minimum send interval limits provider traffic. Notification titles put a short alarm category first so the important text remains visible when WeChat truncates its preview.

Copy `tools/serverchan.secrets.example.ps1` to the ignored `tools/serverchan.secrets.ps1` and enter the SendKey locally. Turbo keys beginning with `SCT` use `sctapi.ftqq.com`. ServerChan 3 keys beginning with `sctp` derive the documented UID endpoint automatically.

Run the gateway with:

```powershell
.\tools\run_phone_alert_gateway.ps1 -StatusUri 'http://192.168.101.19/status'
```

Run its host tests with:

```powershell
.\tests\test_phone_alert_gateway.ps1
```

## Pass criteria

- One confirmed event produces one notification with device identity, state, and timestamp.
- Retries cannot create duplicate alerts.
- A reconnect does not replay stale alerts as new events.
- Secrets are excluded from Git and logs.

## Current status

Provider-independent policy and the ServerChan HTTPS gateway are implemented and host-tested. A dry-run successfully consumed the live Stage 10 endpoint. A ServerChan Turbo SendKey was configured only in the ignored local secrets file, a minimal live delivery returned `code=0`, and the user confirmed receipt in personal WeChat.

A first live hand-motion run delivered real state notifications, but intermittent movement produced RUN/STOP/RUN messages within a few seconds. The gateway was stopped, and the gateway now requires a changed state to remain stable for 10 seconds before it is queued. Stage 11 is not final until a clean live debounce run proves that this correction suppresses short transition noise while still delivering real STOP/RUN events.
