# Stage 8: Mounted machine dataset

## Goal

Collect labeled vibration windows from the final external mounting location while the real machine is known to be stopped and running. Stage 6 hand movement is not a substitute for this dataset.

## Safety boundary

- Mount the sensor externally with the monitored machine powered off and locked out according to site procedure.
- Do not open electrical cabinets or connect to machine terminals, mains voltage, motors, drives, or control wiring.
- Keep the enclosure and USB/power cable away from moving parts, chips, coolant, hot surfaces, and operator controls.
- A responsible site operator must control machine start and stop. The monitor operator must not infer machine state from sound alone.

## Required capture

- At least 60 complete one-second windows labeled `STOP` from the final mounting position.
- At least 60 complete one-second windows labeled `RUN` for every operating mode that must be recognized.
- A separate transition capture covering at least three real STOP-to-RUN and three RUN-to-STOP events.
- Every accepted window must contain 200/200 samples, zero read failures, and zero missed periods.

## Current status

Waiting for supervised access to the real machine, final mounting location, and confirmed machine-state labels. No remote action can safely complete this stage.
