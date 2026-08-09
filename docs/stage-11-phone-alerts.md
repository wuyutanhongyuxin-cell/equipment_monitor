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

## Pass criteria

- One confirmed event produces one notification with device identity, state, and timestamp.
- Retries cannot create duplicate alerts.
- A reconnect does not replay stale alerts as new events.
- Secrets are excluded from Git and logs.

## Current status

Provider-independent policy software implemented and host-testable. Real phone delivery remains blocked on Stage 10 production transport and user selection/configuration of a notification provider. No external account or message was created without authorization.
