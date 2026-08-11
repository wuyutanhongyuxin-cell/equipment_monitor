# Stage 12: Active buzzer bring-up

## Inspected module

The photographed module is labeled `GND`, `I/O`, and `VCC`, with high-level trigger text. The initial test uses 3.3 V power and GPIO25 as an active-high control output.

## Wiring

Disconnect USB power before changing wiring.

| Buzzer module | ESP32 NodeMCU-32 V1.3 |
|---|---|
| `VCC` | 3.3 V rail |
| `GND` | Common GND rail |
| `I/O` | GPIO25 / P25 |

## Test behavior

The isolated sketch drives GPIO25 low before starting Serial, waits two seconds, then produces exactly three 200 ms beeps separated by two seconds. It then holds the output low permanently. This bounded sequence prevents an unattended continuous alarm during bring-up.

## Pass criteria

- The module produces three short, clearly separated beeps.
- It remains silent after the completion message.
- The ESP32 does not reset and the module does not heat excessively.
- GPIO25 low corresponds to off and high corresponds to on.

The buzzer must not be integrated into alarm behavior until this isolated test passes.

## Hardware result

Passed on connected hardware on 2026-08-11. The sketch compiled and uploaded through COM6, the module produced exactly three short beeps, and the user confirmed it remained off afterward. This verifies active-high GPIO25 control at 3.3 V.

The integrated monitor now drives GPIO25 low through an early setup hook before sensor calibration begins. Automatic alarm sounding remains disabled until the site alarm policy and a physical mute mechanism are defined.
