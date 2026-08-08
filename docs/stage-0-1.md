# Stage 0 and Stage 1: Environment and serial test

## Current goal

Verify the complete path before adding any sensor:

```text
Arduino IDE -> compile -> USB/CH340 -> COM6 -> ESP32 -> Serial Monitor
```

No MPU6050 or other module should be connected during these stages.

## Install Arduino IDE

1. On the official [Arduino Software](https://www.arduino.cc/en/software) page, download the current stable **Arduino IDE 2.x** for Windows.
2. Use the Windows 64-bit installer unless a portable ZIP installation is specifically needed.
3. Complete the installer with its default options, then start Arduino IDE.

Arduino IDE 2 requires 64-bit Windows 10 or newer. Do not install a nightly build or the legacy Arduino IDE 1.x for this project.

## Install ESP32 board support

Use the stable package, not the development package.

1. In Arduino IDE, open `File > Preferences`.
2. Find `Additional boards manager URLs`.
3. Add the stable Espressif URL:

   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`

   If the field already contains another URL, use the list button and add this URL as a new line. Do not erase an existing valid URL.
4. Click `OK`.
5. Open `Tools > Board > Boards Manager` (or click the board-manager icon in the left sidebar).
6. Search exactly for `esp32`.
7. Install **esp32 by Espressif Systems**. Choose the current stable version shown by Boards Manager; do not choose a development or `dev` package.
8. Wait until the button/status says `Installed`, then restart Arduino IDE once.

   If the official package URL fails to download from mainland China, use Espressif's official stable China mirror instead:

   `https://jihulab.com/esp-mirror/espressif/arduino-esp32/-/raw/gh-pages/package_esp32_index_cn.json`

   Remove the global ESP32 URL before adding the China mirror so Boards Manager does not select a global package version. Select the newest stable version with the `-cn` suffix. The China mirror must be updated manually because automatic updates target the global package.

## Select the board and port

1. Connect the ESP32 directly to the computer with the known working Micro USB data cable.
2. Open `Tools > Board > esp32 > ESP32 Dev Module`.
3. Open `Tools > Port > COM6 (USB-SERIAL CH340)`.
4. Leave other ESP32 tool settings at their defaults for the first upload.

`ESP32 Dev Module` is the conservative generic selection for an ESP-32E / ESP32-WROOM-class board when the exact development-board vendor and model are unknown.

## Open the test sketch

Open:

`firmware/stage_01_serial_test/stage_01_serial_test.ino`

The sketch uses `Serial.begin(115200)` and prints one counter line per second.

## Compile

1. Click the checkmark **Verify** button at the upper left.
2. Wait for compilation to finish.

Normal result: the output panel finishes without red error text and reports program-storage and dynamic-memory usage. Warnings are not automatically failures; report them if unsure.

## Upload

1. Confirm the selected board is `ESP32 Dev Module` and the selected port is `COM6`.
2. Click the right-arrow **Upload** button.
3. Do not press BOOT preemptively.

Normal upload output usually includes compilation, a connection attempt, chip information, flash writing progress such as `Writing at ...`, verification, and a reset message. The exact wording varies with the installed ESP32 package version.

Upload success is indicated by Arduino IDE reporting that uploading completed successfully, without a fatal error. A brief change in the board LEDs during upload is normal.

## Read Serial Monitor

1. Open `Tools > Serial Monitor`, or click the Serial Monitor icon near the upper-right corner.
2. Set its baud-rate dropdown to `115200`.
3. If the boot line is missing because the monitor opened after startup, press the board's **EN/RESET** button once. Do not press BOOT.

Expected output:

```text
ESP32 monitor boot OK
counter: 1
counter: 2
counter: 3
```

The counter continues once per second. A few boot-ROM characters before these lines are not a problem if the required lines are readable at 115200 baud.

## Pass criteria

Stage 0 and Stage 1 pass only when all of these are true:

- Arduino IDE can compile the sketch for `ESP32 Dev Module`.
- Arduino IDE uploads it through `COM6`.
- Serial Monitor at 115200 shows the boot message and an increasing counter once per second.

Do not connect the MPU6050 until these results are confirmed.
