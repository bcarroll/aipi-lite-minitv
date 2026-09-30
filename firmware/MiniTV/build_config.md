# Firmware Build Configuration

## Approved production toolchain and libraries

The user approved the exact dependency set recorded in
[`docs/DEPENDENCIES.md`](../../docs/DEPENDENCIES.md):

- Arduino-ESP32 core 2.0.17
- Arduino_GFX 1.6.0
- JPEGDEC 1.8.4
- minimp3 commit `ea99364f61c14656440e8d77e9c233ccf3124633`
- ArduinoJson 7.4.3

Use Arduino IDE 2.x with the Arduino-ESP32 boards package pinned to 2.0.17.
Install the four libraries at the exact approved versions above. Do not accept
automatic upgrades for a reproducible build. The core supplies `WiFi`,
`HTTPClient`, `WebServer`, `Wire`, `SPI`, and I2S APIs.

## Board settings

Select an ESP32-S3 target with 16 MB flash and 8 MB Octal PSRAM compatible with
`ESP32_GENERIC_S3-SPIRAM_OCT`. Select the flash size and PSRAM mode to match the
hardware. Do not select or install a partition scheme for a firmware that will
access the existing MicroPython data drive until the actual device partition
table and filesystem have been read and recorded in
[`docs/COMPATIBILITY.md`](../../docs/COMPATIBILITY.md).

This scaffold does not define a partition CSV, invoke a filesystem library,
flash firmware, or access media. Build and device upload remain unverified;
`arduino-cli` and PlatformIO are not installed in the current environment.

## Sketch layout

Open `MiniTV.ino` from this directory in Arduino IDE. Keep all sketch sources
within the `MiniTV` sketch folder. The initial sketch only starts serial
diagnostics. It does not initialize GPIO10, mount storage, configure display or
audio hardware, connect to Wi-Fi, or write flash.
