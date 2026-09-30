# AI-PI Lite MiniTV

An ESP32-S3 port of the Wi-Fi-managed CYD MiniTV for the XORIGIN AI-PI Lite
XY006PL01. The board has a 128 x 128 ST7735-compatible display and ES8311
audio codec. It has no external SPI microSD module; local videos are intended
to remain on the MicroPython data drive.

## Current implementation status

The firmware currently contains an AI-PI Lite profile, display status layer,
GPIO42 button handling, bounded local media and playlist readers, and an
accessible dashboard page shell. MJPEG/MP3 playback, audio output, Wi-Fi
streaming, and dashboard API routes are not implemented yet.
The current partition table, filesystem type, and native mount compatibility
are unknown. The firmware therefore does not mount, format, erase, migrate, or
write the MicroPython data drive. See [filesystem compatibility](docs/COMPATIBILITY.md)
and [firmware build notes](firmware/MiniTV/build_config.md).

## Hardware behavior so far

The display uses GPIO3 for backlight, GPIO7 D/C, GPIO15 CS, GPIO16 SCLK,
GPIO17 MOSI, and GPIO18 reset. The active-low function button is on GPIO42.
The status display identifies status and detail with text. Rotation and color
order follow the existing AI-PI Lite MicroPython implementation, but physical
orientation and color have not been observed on the device.

GPIO10 is reserved for board power control and is never initialized by this
firmware. Audio pins are documented in `aipi_lite_profile.h`; audio hardware
is not initialized.

## Playlist format

The parser accepts `/Videos/playlist.json`, with a maximum size of 4096 bytes
and up to 32 channels. IDs are unique and contain only letters, digits,
hyphens, and underscores. Titles are limited to 48 characters. Local paths are
restricted to `/Videos/` and matching `.mjpeg` or `.mp3` extensions. Network
entries require direct HTTP(S) resource URLs with those extensions.

```json
{
  "version": 1,
  "channels": [
    {
      "id": "local-show",
      "title": "Local show",
      "source": "file",
      "video": "/Videos/1/Show.mjpeg",
      "audio": "/Videos/1/Show.mp3"
    },
    {
      "id": "relay-show",
      "title": "Relay stream",
      "source": "http",
      "video": "https://media.example.org/show.mjpeg",
      "audio": "https://media.example.org/show.mp3"
    }
  ]
}
```

YouTube watch pages and SMB/NFS shares are not media streams supported by the
device. A PC or network relay must expose compatible MJPEG and MP3 resources
over HTTP(S). Do not put credentials or signed URLs in a checked-in playlist.

## Dependencies and build

The user-approved dependency pins and their licenses are listed in
[`docs/DEPENDENCIES.md`](docs/DEPENDENCIES.md). Follow
[`firmware/MiniTV/build_config.md`](firmware/MiniTV/build_config.md) to set up
the ESP32-S3 toolchain. Do not choose a partition scheme for installation
until the device's existing partition map and filesystem have been inspected.

## Verification status

Only source review and `git diff --check` have been performed in this workspace.
No host tests or Arduino target build have been run, and no device has been
flashed or accessed. See the implementation plan at
[`docs/superpowers/plans/2026-09-30-aipi-lite-minitv.md`](docs/superpowers/plans/2026-09-30-aipi-lite-minitv.md).
