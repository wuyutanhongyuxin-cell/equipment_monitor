# Stage 0/1 troubleshooting

Preserve and report the complete error text. Diagnose the category before pressing buttons or changing settings.

## `Failed to connect`, repeated `Connecting...`, or `Timed out waiting for packet header`

1. Confirm the IDE still shows `ESP32 Dev Module` and `COM6`.
2. Close Serial Monitor and any other serial-terminal program, then retry Upload once.
3. Disconnect only external wires/modules if any were attached; Stage 0/1 should have USB only.
4. Press **EN/RESET** once and retry.
5. Only if the same connection error remains: start Upload, wait until `Connecting...` appears, hold **BOOT**, release it as soon as flash writing begins.

This last step manually enters the ROM download mode. It is not needed on boards whose auto-reset circuit works.

## `COM port unavailable`, `Access is denied`, or port disappears

1. Close Serial Monitor and all other programs that may own COM6.
2. Unplug the USB cable, wait five seconds, reconnect it, and check Windows Device Manager again.
3. Re-select the port because Windows may assign a different COM number.
4. If Device Manager shows a warning icon or no CH340 device, repair/install the CH340 driver from the board seller's documented source or WCH's official driver source.

## Wrong board or chip-family error

If output says the connected chip is ESP32 but the selected target is ESP32-S2, ESP32-C3, or another family, select `ESP32 Dev Module` and compile/upload again. Do not select a more specific board merely because its name looks similar.

## USB cable or USB port problem

A power-only cable can light the red LED but cannot transfer data. In this case, however, Windows already detecting `USB-SERIAL CH340 (COM6)` is strong evidence that the current cable carries data. Reconsider the cable only if COM6 becomes intermittent or disappears. Connect directly to the computer and avoid an unpowered USB hub during diagnosis.

## Upload succeeds but Serial Monitor is blank

1. Set Serial Monitor to `115200`.
2. Confirm it is attached to COM6.
3. Press **EN/RESET** once.
4. Wait at least three seconds for counter lines.

## Serial text is garbled

Set Serial Monitor to `115200`. The sketch and monitor must use the same baud rate. Then press **EN/RESET** once.

## Compile fails before any connection attempt

This is not a COM-port or BOOT problem. Copy the first error and the final error from the output panel. Common causes are an incomplete `esp32 by Espressif Systems` installation, the wrong sketch file, or a damaged package download.

## Arduino IDE remains on the startup logo

Capture the IDE startup log before deleting caches or reinstalling. A confirmed cause on the verified Windows computer was a user-level HTTP proxy without a localhost bypass:

```text
HTTP_PROXY=http://127.0.0.1:10811
HTTPS_PROXY=http://127.0.0.1:10811
NO_PROXY was not set
```

The IDE starts `arduino-cli daemon` on `127.0.0.1` and communicates with it over gRPC. Sending that local connection through the HTTP proxy caused this log error and left the loading logo visible:

```text
14 UNAVAILABLE: No connection established
read ECONNRESET
```

Keep the external proxy and add this Windows user environment variable:

```text
NO_PROXY=127.0.0.1,localhost
```

Then close the entire stuck Arduino IDE process tree and start it again. A successful startup log reaches:

```text
Replace loading indicator with ready workbench UI
Changed application state from 'initialized_layout' to 'ready'
```

Do not apply this fix merely because a splash screen is slow. Confirm the proxy variables and `ECONNRESET` log signature first.
