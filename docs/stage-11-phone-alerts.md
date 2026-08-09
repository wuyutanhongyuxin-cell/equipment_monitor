# Stage 11: Phone alerts

## Goal

Send actionable phone notifications for confirmed state transitions and sensor/offline faults, with deduplication and rate limiting.

## Required decisions

- Approved notification provider and recipient account/device.
- Which transitions alert, allowed delay, quiet hours, and escalation rules.
- Offline/fault timeout and recovery notification behavior.

## Pass criteria

- One confirmed event produces one notification with device identity, state, and timestamp.
- Retries cannot create duplicate alerts.
- A reconnect does not replay stale alerts as new events.
- Secrets are excluded from Git and logs.

## Current status

Blocked on Stage 10 transport and user selection/configuration of a notification provider. No external account or message was created without authorization.
