# Stage 4: MPU6050 acceleration reading

## Goal

Confirm that the verified MPU6050 can be identified, awakened, and read continuously over I2C. This stage reads acceleration only; it does not classify machine state yet.

## Wiring and safety

Keep the Stage 3 wiring unchanged:

| ESP32 | GY-521 |
|---|---|
| `3V3` | `VCC` |
| `GND` | `GND` |
| `P21` | `SDA` |
| `P22` | `SCL` |

Disconnect USB before touching any wire. This remains a low-voltage bench test; do not connect the monitor to industrial equipment or mains voltage.

## Test procedure

1. Open `firmware/stage_04_accel_read/stage_04_accel_read.ino`.
2. Select `ESP32 Dev Module` and `COM6`.
3. Compile and upload, then open Serial Monitor at 115200 baud.
4. Leave the breadboard stationary on a flat surface for at least 30 seconds.
5. Confirm that no `read: failed` line appears.

The sketch uses the built-in `Wire` library, so no external Arduino library is required. It verifies `WHO_AM_I`, wakes the sensor, and reads the default +/-2g acceleration range at two samples per second.

## Expected output

```text
ESP32 MPU6050 acceleration test boot OK
MPU6050 WHO_AM_I=0x68
MPU6050 init: ready, accel_range=+/-2g
accel_g: x=+0.012, y=-0.021, z=+0.986, magnitude=0.986
```

The axis carrying gravity depends on board orientation. A stationary sensor should have a total magnitude near 1g, allowing for sensor offset and mounting angle.

## Pass criteria

- `WHO_AM_I` is standard `0x68` or the tested module's explicitly reported compatible value `0x74`.
- Initialization reaches `MPU6050 init: ready`.
- At least 30 seconds of continuous samples contain no read failure.
- Stationary acceleration magnitude remains plausibly near 1g (accepted bring-up range: 0.8g to 1.2g).

Do not begin Stage 5 until these criteria pass on the real hardware.

The tested module was later found to return `WHO_AM_I=0x74`, which is not the official MPU-6050 value. Stage 4 therefore verifies compatible acceleration behavior, not authentic TDK/InvenSense device identity.
