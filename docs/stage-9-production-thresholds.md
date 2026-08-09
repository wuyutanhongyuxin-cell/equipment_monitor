# Stage 9: Production threshold calibration

## Goal

Derive candidate production thresholds from the Stage 8 mounted dataset, then validate them on held-out machine runs. Bench hand-movement thresholds are forbidden as production defaults.

## Analyzer

Run:

```powershell
.\tools\analyze_threshold_dataset.ps1 -Path <captured-serial-log>
```

The input must contain Stage 6-style labeled `data` lines. `STILL`/`STOP` normalize to STOP and `VIBRATION`/`RUN` normalize to RUN. Invalid or incomplete windows are rejected.

The analyzer reports sample counts, ranges, means, empirical 95th/5th percentiles, overlap, and a midpoint only when the observed full ranges are separated. It defaults to requiring 60 windows per class.

## Pass criteria

- Stage 8 data requirements are met.
- Training and held-out validation captures are kept separate.
- RUN and STOP thresholds plus confirmation durations are selected from mounted data.
- Held-out transition tests meet the agreed false-positive, false-negative, and detection-delay limits.

## Current status

Analyzer implemented and locally fixture-tested. Production calibration is blocked on Stage 8 data and acceptance limits from the machine owner.
