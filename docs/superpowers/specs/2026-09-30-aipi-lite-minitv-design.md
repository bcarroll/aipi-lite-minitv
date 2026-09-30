# AI-PI Lite MiniTV Port Design

**Status:** Approved and reviewed by the user
**Date:** 2026-09-30

## Goal

Adapt the Wi-Fi-managed ESP32 CYD MiniTV project for the XORIGIN AI PI-Lite
(model XY006PL01). The device will play local videos from its existing
MicroPython data filesystem and compatible network media sources selected by a
JSON playlist. The port must use the AIPI Lite display, audio codec, and
function button.

The upstream project targets a classic ESP32 CYD with a 320 x 240 ILI9341
display, onboard microSD, and GPIO DAC. The AIPI Lite instead uses an ESP32-S3,
128 x 128 ST7735-compatible display, ES8311 codec, and has no documented
onboard SD slot. The upstream firmware is therefore a feature and decoder
reference, not a drop-in firmware image.

## Chosen Approach

Use a native ESP32-S3 C++ firmware port, built in phases. This keeps the
upstream project's native JPEG and MP3 decoding approach while allowing direct
use of the AIPI Lite SPI display and I2S codec. Do not rewrite the media decoder
path in MicroPython.

The firmware must identify and mount the existing MicroPython data partition
without formatting it. Before enabling writes, prove that the filesystem type,
partition offset, and size are compatible with the selected native runtime. If
it cannot mount the data partition safely, stop and report the incompatibility;
do not erase, reformat, or silently migrate user files. The firmware packaging
must preserve the existing partition layout or document an explicit,
recoverable migration path for user approval.

## Hardware Profile

| Function | AIPI Lite connection |
| --- | --- |
| Display | 128 x 128 ST7735-compatible TFT |
| Display SPI | SCLK GPIO16, MOSI GPIO17, CS GPIO15, D/C GPIO7, RESET GPIO18 |
| Backlight | GPIO3 |
| Audio codec | ES8311; I2C SCL GPIO4, SDA GPIO5; I2S MCLK GPIO6, DOUT GPIO11, LRCLK GPIO12, DIN GPIO13, BCLK GPIO14 |
| Speaker amplifier gate | GPIO9 |
| Playback button | GPIO42, active-low right function button |

GPIO10 board power control is out of scope and must not be driven. GPIO46 status
LED support is optional and must not block playback. Display orientation and
color order follow the existing AIPI Lite display implementation and require
physical confirmation on the device.

## Media and Playlist

Local media retains the MiniTV-compatible pairing: an MJPEG video and matching
MP3 audio file share a base name. Numbered directories form channels, with an
optional `random` directory. Files remain in the device's existing data
filesystem; no SD card is assumed.

Add a JSON playlist file under `/Videos/playlist.json` to name channels and
select local or network media. The format will be versioned and validated
before playback. Local entries refer to files in the device media tree. Network
entries provide direct URLs to compatible MJPEG and MP3 resources. Network
playback must stream bounded chunks and report buffering, HTTP, decode, and
codec errors on screen and in diagnostics. Credentials and signed URLs must
not be committed in examples or logs.

The player does not resolve YouTube watch pages or mount SMB/NFS shares itself.
Those sources require a PC or network relay that exposes compatible media
resources over HTTP(S). The device must reject unsupported content types and
show a useful error rather than treating a web page as a media stream.

Use the AIPI Lite's existing private 2.4 GHz Wi-Fi setup conventions. Network
streaming is opt-in through a playlist entry. The device will not add
telemetry, cloud accounts, or public internet administration. The web control
interface must remain on the local network and must not be exposed directly to
the public internet.

## User Interface and Controls

Retain the upstream MiniTV functions where they fit the AIPI Lite: channel
selection, random playback, mute, volume and brightness controls, sleep/restart,
channel and file management, uploads, settings, diagnostics, and firmware
updates. Map physical controls to GPIO42 using debounced short, double, and
long presses. The small screen must show channel/playback state and explicit
text for errors; color alone cannot communicate status.

The browser dashboard will be responsive and keyboard-operable. All controls
need programmatic labels, visible focus, adequate contrast, and status text
that does not rely on color alone. Target applicable Section 508 and WCAG 2.2 AA
accessibility criteria; do not claim certification without a formal audit.

## Phased Delivery

1. **Hardware and local playback:** establish the native build, safe partition
   discovery/mounting, display output, GPIO42 control, ES8311 setup, and one
   local MJPEG/MP3 pair. No partition formatting or automatic migration.
2. **Playlist and network sources:** validate the versioned JSON playlist and
   play compatible local HTTP(S) media with bounded buffering and diagnostics.
3. **Management interface:** adapt the Wi-Fi setup, browser playback controls,
   channel/file management, media upload, settings, and diagnostics to the
   filesystem-backed library.
4. **Maintenance:** adapt OTA and backup/restore only after the partition map,
   recovery workflow, and interrupted-update behavior are verified. Preserve a
   USB recovery path and document how to restore the original MicroPython
   firmware and files.

## Verification

- Build against the selected ESP32-S3 target and pinned library versions.
- Exercise playlist parsing with valid, malformed, oversized, and unsupported
  entries using host-side tests.
- Verify partition identification and mount behavior before allowing writes;
  verify existing files remain readable after firmware installation.
- On hardware, test display orientation/color, button debouncing, codec
  initialization, audible MP3 output, local MJPEG/MP3 playback, Wi-Fi loss and
  recovery, and HTTP stream failures.
- Test dashboard keyboard access, labels, focus, contrast, and explicit status
  text. Automated checks are evidence only and do not establish Section 508
  certification.
- Do not claim hardware playback, partition preservation, or production
  readiness until observed on the physical device.

## Constraints and Open Implementation Checks

- The AIPI Lite has 16 MB flash and 8 MB PSRAM, but the usable data filesystem
  size and partition layout must be measured from the actual MicroPython
  installation before setting media limits.
- Native dependencies for JPEG decoding, MP3 decoding, display rendering,
  filesystem mounting, networking, and the web UI must be pinned and reviewed
  before addition. No new production dependency is added until the required
  approval is obtained.
- Confirm ES8311 clock/register settings against the known working AIPI Lite
  firmware. Existing codec address detection is evidence for I2C presence, not
  proof of audible playback.
- The design provides no claim of federal procurement, FIPS, FedRAMP, FCC, UL,
  or supply-chain certification. Applicable requirements need independent
  evidence before federal deployment.
- Keep stock firmware backup and restore instructions available before any
  operation that overwrites the device firmware.

## Upstream Reference

- [DynaMight1124's ESP32-MiniTV-Player](https://github.com/DynaMight1124/ESP32-MiniTV-Player)
  is the button-driven playback base: numbered and random channels, MJPEG with
  paired MP3/AAC audio, and separate audio and video task modules. Its supplied
  board profiles target SD storage, an ST7789 plus external I2S DAC, or a CYD
  ILI9341 plus classic ESP32 internal DAC. The MiniTV media/channel behavior
  transfers conceptually, but the SD access and hardware drivers do not.
- [nickm324's ESP32-CYD-MiniTV](https://github.com/nickm324/ESP32-CYD-MiniTV)
  builds on that player with the Wi-Fi dashboard, uploads, settings,
  diagnostics, and OTA workflow. It remains targeted at CYD/microSD hardware
  and classic ESP32 APIs, so its management features need adaptation too.
- AIPI Lite pins and component assumptions come from the supplied
  `SPEC.md` and existing AI-PI Lite firmware sources.

Implementation consequence: retain compatible MJPEG/MP3 media semantics and
use the upstream task split as a concurrency reference, while implementing
AI-PI Lite display, ES8311 audio, data-drive access, and network streaming
against their own verified interfaces. No upstream SD mount or board profile
is carried over unchanged.
