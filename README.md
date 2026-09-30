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
firmware. The ES8311 driver uses the existing AI-PI Lite 16 kHz, 16-bit I2S
register sequence and keeps the GPIO9 amplifier gate low until playback is
active. The MCLK pin (GPIO6) is left unused because this sequence derives the
codec clock from BCLK. Audio playback is not yet integrated with the media
player; physical codec operation is unverified.

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

## Upstream basis

The player lineage begins with [DynaMight1124's ESP32-MiniTV-Player](https://github.com/DynaMight1124/ESP32-MiniTV-Player), which provides the button-driven channel model, MJPEG/MP3 pairing, random playback, and separate audio and video task structure. Its documented hardware profiles use an SD card and either an ST7789 display with external I2S audio or a CYD ILI9341 with the classic ESP32 DAC. Its SD-specific access and hardware profiles do not fit the AI-PI Lite directly.

[nickm324's ESP32-CYD-MiniTV](https://github.com/nickm324/ESP32-CYD-MiniTV) builds on that player and adds the Wi-Fi dashboard, uploads, settings, diagnostics, and OTA workflow. It also remains CYD and microSD oriented. This port keeps those projects as behavioral references while adapting storage to the existing MicroPython data drive and hardware to the AI-PI Lite's ST7735-compatible display and ES8311 codec.

Desktop conversion is an accepted way to prepare media, including videos obtained from YouTube. [DynaMight1124's MiniTV-Video-Converter](https://github.com/DynaMight1124/MiniTV-Video-Converter) can batch-convert common desktop video formats into the upstream MiniTV file layout, with options for target screen and audio format, and its README describes a YouTube URL conversion workflow. Transfer its resulting paired files into the AI-PI Lite data drive for local playback. Treat its presets as starting points: the AI-PI Lite's 128 x 128 display and 16 kHz codec path need a separately verified encoding profile. A YouTube watch URL is not itself a direct media stream supported by the device playlist.

This repository includes a repo-native desktop utility at [`tools/minitv_converter.py`](tools/minitv_converter.py), compatible with the MiniTV file layout. Choose the AI-PI Lite profile to create numbered channel folders with paired `.mjpeg` and 16 kHz mono `.mp3` files. That profile is an initial encoding target and still needs playback verification on the board. Choose the existing `/Videos` data-drive directory as output only after confirming it is mounted and accessible from the desktop; otherwise use a staging folder and transfer the generated numbered folders later. Existing outputs are skipped rather than overwritten.

### Desktop setup

The desktop GUI requires Python 3.10 or newer with Tk support and FFmpeg on
`PATH`. Python's standard library supplies the GUI and conversion driver; there
are no required pip packages for converting local files. Test Tk support with
`python3 -m tkinter`. Install FFmpeg using your operating system's package
manager, or follow the [FFmpeg download page](https://ffmpeg.org/download.html)
for executable builds, then confirm `ffmpeg -version` works in a new terminal.

Optional URL downloads use yt-dlp. Create a virtual environment, activate it,
and install the pinned optional Python package:

macOS or Linux:

```sh
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r tools/requirements-converter.txt
python tools/minitv_converter.py
```

Windows PowerShell:

```powershell
py -3 -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install -r tools/requirements-converter.txt
python -m tkinter
python tools\minitv_converter.py
```

The `yt-dlp[default]` package is pinned in
[`tools/requirements-converter.txt`](tools/requirements-converter.txt). See
the [yt-dlp installation guide](https://github.com/yt-dlp/yt-dlp/wiki/Installation)
for non-pip installation options. Local conversion does not require yt-dlp.
On macOS, `brew install ffmpeg` installs FFmpeg with Homebrew; on Debian or
Ubuntu, use `sudo apt-get update && sudo apt-get install ffmpeg`. On Windows,
install a build linked from the [FFmpeg download page](https://ffmpeg.org/download.html)
and add its `bin` directory to `PATH`.

### Docker setup

The root [`Dockerfile`](Dockerfile) builds a headless converter image containing
Python, FFmpeg, and the pinned yt-dlp package. It does not need Python, FFmpeg,
yt-dlp, or Tk on the host. Docker mode accepts local video folders or a URL
through the same script's command-line interface.

```sh
mkdir -p media converted
docker build -t aipi-lite-minitv-converter .
docker run --rm \
  --user "$(id -u):$(id -g)" \
  -v "$PWD/media:/input:ro" \
  -v "$PWD/converted:/output" \
  aipi-lite-minitv-converter \
  --input /input --output /output --profile aipi-lite
```

The input mount is read-only. Numbered output folders are written to `converted`.
To download and convert one HTTP(S) URL, mount the output folder and run:

```sh
docker run --rm \
  --user "$(id -u):$(id -g)" \
  -v "$PWD/converted:/output" \
  aipi-lite-minitv-converter \
  --url "https://example.org/video" --output /output --profile aipi-lite
```

On Windows PowerShell, omit the `--user` option and use host paths like this:

```powershell
docker run --rm `
  -v "${PWD}/media:/input:ro" `
  -v "${PWD}/converted:/output" `
  aipi-lite-minitv-converter `
  --input /input --output /output --profile aipi-lite
```

Add FFmpeg or yt-dlp only to the Dockerfile or requirements file when changing
the pinned image environment.

## Verification status

The converter passed a Python syntax compilation check and `git diff --check`.
No Docker image build, converter runtime test, host test, or Arduino target
build has been run, and no device has been flashed or accessed. See the
implementation plan at
[`docs/superpowers/plans/2026-09-30-aipi-lite-minitv.md`](docs/superpowers/plans/2026-09-30-aipi-lite-minitv.md).
