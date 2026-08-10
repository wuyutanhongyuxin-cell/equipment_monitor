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

## Dual-channel test wiring

After both isolated tests pass, power both modules from the same 3.3 V and ground rails. Connect S1 AO to ADC1 GPIO34 and S2 AO to ADC1 GPIO35; leave both DO pins disconnected. The dual test sketch discards the first conversion after changing channels, then averages 16 raw conversions per channel to reduce ADC switching and noise effects.

## Dual-channel result

Passed on connected hardware on 2026-08-11. Both modules shared the ESP32 3.3 V and ground rails while S1 AO used GPIO34 and S2 AO used GPIO35.

| Condition | S1 raw ADC | S2 raw ADC |
|---|---:|---:|
| Both in room light | 527-532, mean 530 | 461-466, mean 463 |
| S1 covered, S2 exposed | 2939-3272, mean 3141 | 675-757, mean 705 |
| S1 exposed, S2 covered | 396-409, mean 404 | 3932-3953, mean 3942 |
| Both covered | 3186-3525, mean 3430 | 4018-4095, mean 4083 |

The exposed channel remained far below the covered channel in both one-at-a-time tests, and both channels reached high values together without reset or supply failure. Small exposed-channel changes are expected from hand shadows and ambient-light movement. No significant electrical channel coupling was observed.
