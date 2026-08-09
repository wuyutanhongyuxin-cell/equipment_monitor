# Stage 6: Threshold dataset collection

## Goal

Collect labeled stationary and deliberately disturbed vibration windows using the verified Stage 5 pipeline. A threshold candidate is reported only if all collected vibration RMS values exceed all stationary RMS values.

## Procedure

1. Keep the sensor on a stable surface and upload `firmware/stage_06_threshold_dataset/stage_06_threshold_dataset.ino`.
2. Open serial at 115200 baud and wait for `Dataset: ready; send g to start`.
3. Send the single character `g`.
4. Keep the board untouched during the 15-second `STILL` phase.
5. Wait through the 10-second `PREPARE` countdown.
6. When the onboard LED turns on, repeatedly tap the table beside the breadboard or gently move the whole breadboard for the full 15-second `VIBRATION` phase. Do not touch wires or pins.
7. Stop when the LED turns off and retain all labeled output and summaries.

## Pass criteria

- Both labels contain exactly 15 complete windows.
- Every collected window has `valid=200/200`, `failed=0`, and `missed=0`.
- Sampling rate remains between 199.0 Hz and 201.0 Hz.
- The vibration dataset is materially higher than the stationary baseline.
- A threshold is accepted only if `vibration rms_min > still rms_max`; otherwise repeat with a more representative vibration method or defer threshold selection.

The resulting threshold is a bench-test candidate only. It must not be treated as a production machine threshold until data is collected from the real mounting location and actual machine RUN/STOP states.
