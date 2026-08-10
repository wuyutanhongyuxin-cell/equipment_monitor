# Stage 12: LM393 light sensor bring-up

## Inspected module

The photographed module is a common LM393 light sensor board with labeled `AO`, `DO`, `GND`, and `VCC` pins. The first test uses one module, 3.3 V power, and analog output only. `DO` and the threshold potentiometer are not part of this test.

## Safe wiring

Disconnect USB power before changing wiring.

| LM393 module | ESP32 NodeMCU-32 V1.3 | Purpose |
|---|---|---|
| `VCC` | `3V3` | 3.3 V power |
| `GND` | `GND` | Common ground |
| `AO` | `P34` / GPIO34 | ADC1 analog input |
| `DO` | Not connected | Unused |

GPIO34 is input-only and belongs to ADC1, so it does not conflict with WiFi ADC2 restrictions. Never power this test module from 5 V while its output is connected to the ESP32.

## Test procedure

1. Recheck every label with USB disconnected.
2. Open `firmware/stage_12_light_sensor_test/stage_12_light_sensor_test.ino`.
3. Select `ESP32 Dev Module` and `COM6`, then compile and upload.
4. Open Serial Monitor at 115200 baud.
5. Record readings under normal room light, with the photoresistor covered, and with a phone flashlight aimed at it from a fixed distance.

## Pass criteria

- Boot text identifies GPIO34 and 3.3 V power.
- `light_raw` and `light_mv` remain within valid ADC range without wiring faults.
- Covering and illuminating the photoresistor produce repeatable, clearly separated readings.
- No ESP32 reset, excessive module heating, or unstable USB connection occurs.

Do not connect the second light sensor or integrate WiFi/state classification until this isolated test passes.

## First-module result

Passed on GPIO34 at 3.3 V on 2026-08-10. Twenty-six readings were captured for each condition:

| Condition | Raw ADC result | Millivolt estimate |
|---|---:|---:|
| Normal room light | 587-591 | 621-623 mV |
| Photoresistor fully covered | 3555-3607, mean 3574 | 2903-2925 mV, mean 2911 mV |
| Phone flashlight at about 10 cm | 0 | 142 mV |

The module is inverse-reading: darker conditions produce a higher AO value. The flashlight drove the input to the low end of the ADC range. The calibrated millivolt helper has a non-zero estimate at raw zero, so classification and calibration use raw ADC readings as the primary signal.

The first sensor passed the isolated electrical and response test.

The second module then passed the same isolated GPIO34 test with 23 room-light samples and 26 samples for each other condition:

| Condition | Raw ADC result | Millivolt estimate |
|---|---:|---:|
| Normal room light | 637-640, mean 639 | 662-664 mV, mean 663 mV |
| Photoresistor fully covered | 3113-3156, mean 3127 | 2644-2673 mV, mean 2654 mV |
| Phone flashlight at about 10 cm | 0 | 142 mV |

Both modules have the same inverse response but different baseline and covered values. Each channel therefore requires independent calibration. Simultaneous two-channel wiring and firmware remain untested.
