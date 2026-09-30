# Proposed MiniTV Production Dependencies

**Approval status:** Awaiting user approval. No production packages or source
code have been added to this repository.

The upstream MiniTV uses the classic ESP32 CYD. The versions below are a
reviewed proposal for an ESP32-S3 port; they still require build and physical
playback validation. The upstream Helix decoder path is not proposed because
the currently referenced `arduino-libhelix` redirects to `codec-helix`, whose
GPL-3.0 repository and separate RealNetworks decoder licensing need legal
review. `minimp3` is proposed instead, subject to performance verification on
the target.

## Exact proposed production set

| Package | Pin | Purpose | License | Source and notes |
| --- | --- | --- | --- | --- |
| Arduino-ESP32 core | `2.0.17` | ESP32-S3 board support, SPI, Wi-Fi, HTTP, web server, I2S and filesystem framework | Apache-2.0 plus bundled component notices | [Release](https://github.com/espressif/arduino-esp32/releases/tag/2.0.17). Upstream MiniTV's tested core; must confirm board build configuration. |
| Arduino_GFX | `1.6.0` | ST7735-compatible TFT drawing | BSD-2-Clause | [Repository](https://github.com/moononournation/Arduino_GFX), [license](https://github.com/moononournation/Arduino_GFX/blob/master/license.txt). Pin matches the [upstream MiniTV README](https://github.com/nickm324/ESP32-CYD-MiniTV#what-you-need); upstream states ESP32-S3 is not drop-in compatible, so target adaptation remains required. |
| JPEGDEC | `1.8.4` | JPEG frame decode for MJPEG playback | Apache-2.0 | [Release](https://github.com/bitbank2/JPEGDEC/releases/tag/1.8.4), [repository](https://github.com/bitbank2/JPEGDEC). Candidate includes ESP32-S3 support; verify its streaming/frame API during implementation. |
| minimp3 | commit `ea99364f61c14656440e8d77e9c233ccf3124633` | MP3 frame decode | CC0-1.0 | [Pinned source](https://github.com/lieff/minimp3/blob/ea99364f61c14656440e8d77e9c233ccf3124633/minimp3.h), [repository](https://github.com/lieff/minimp3). Single-header decoder; its documentation recommends buffering about 10 frames (~16 KB) for reliable synchronization. Target performance remains unverified. |
| ArduinoJson | `7.4.3` | Bounded versioned JSON playlist parsing | MIT | [Release](https://github.com/bblanchon/ArduinoJson/releases/tag/v7.4.3). Current reviewed release candidate; apply explicit input and document limits. |

## Provided by the core or standard toolchain

The following are not proposed as separately installed production libraries:

- `WiFi`, `HTTPClient`, `WebServer`, `Wire`, `SPI`, and I2S APIs from the
  selected Arduino-ESP32 core.
- FAT/VFS and LittleFS implementation support bundled with the framework, if
  the exact MicroPython filesystem can be mounted safely. This compatibility
  is currently **unknown**; no filesystem bridge package is approved or
  selected.
- C++ standard library and Arduino build tools. The exact CLI/toolchain version
  should be pinned in the build configuration after the core is approved.
- Host-side tests should use the repository's existing compiler and Python
  standard library; no test-only production dependency is proposed.

## Approval request

Please approve or revise this exact production set before it is added to a
firmware manifest or build. Approval covers only the listed versions and
licenses. It does not establish filesystem compatibility or hardware
performance. No library declaration, download, vendoring, or installation has
been made.
