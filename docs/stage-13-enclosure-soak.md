# Stage 13: Enclosure and soak validation

## Goal

Validate the complete low-voltage monitor in its final enclosure and external mounting position before operational use.

## Required validation

- Strain relief, insulation, fastening, ventilation, ingress exposure, and service access inspection.
- At least 24 hours powered bench soak followed by a site-defined mounted soak period.
- Repeated power loss, WiFi loss, sensor disconnect, and recovery tests.
- Comparison against operator-confirmed machine state across representative production cycles.
- Final documentation of power source, mounting, maintenance, limitations, and removal procedure.

## Pass criteria

- No unsafe temperature, odor, reset loop, loose conductor, or exposed energized contact.
- No missed or duplicate state transition alerts in the agreed validation set.
- Faults are visible locally/remotely and recover according to specification.
- Site owner accepts the external mounting and operational limitations.

## Log analyzer

`tools/analyze_soak_log.ps1` checks Stage 7-style logs for the minimum complete-window count, incomplete windows, read failures, missed periods, state counts, and RMS range. Its default minimum is 86,400 one-second windows for a 24-hour run.

Run its device-independent regression suite with:

```powershell
.\tests\test_soak_analyzer.ps1
```

## Current status

Soak-log analyzer implemented; physical soak remains blocked on completion of Stages 8-12, enclosure selection, and supervised installation. This project remains low-voltage and external only.
