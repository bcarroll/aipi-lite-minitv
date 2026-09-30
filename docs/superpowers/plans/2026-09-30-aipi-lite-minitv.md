# AI-PI Lite MiniTV Implementation Plan

> **For agentic workers:** Implement this plan inline, one task at a time. Do not flash or otherwise write to a physical device without an explicit instruction. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the Wi-Fi-managed ESP32 CYD MiniTV to AI-PI Lite hardware, preserving and playing local media from the existing MicroPython data filesystem and supporting compatible HTTP(S) media named in a JSON playlist.

**Architecture:** Build a native ESP32-S3 application with separate AIPI Lite hardware, filesystem, media-source, playback, playlist, and web-management modules. First establish safe read-only access to the current MicroPython data partition; add writes only after a verified compatible mount is demonstrated. Deliver local playback before HTTP streaming, management pages, and maintenance functions.

**Tech Stack:** ESP32-S3 Arduino core 2.0.17 as used by the upstream firmware; C++ firmware; user-approved production dependencies Arduino_GFX 1.6.0, JPEGDEC 1.8.4, minimp3 at a reviewed commit, and ArduinoJson 7.4.3; ESP-IDF filesystem support only if verified compatible with the MicroPython partition; host-side C++ tests plus Python standard-library tests for web-asset accessibility.

**Spec:** `docs/superpowers/specs/2026-09-30-aipi-lite-minitv-design.md`

## Global Constraints

- Target XORIGIN AI PI-Lite XY006PL01: ESP32-S3, 16 MB flash, 8 MB PSRAM, 128 x 128 ST7735-compatible display, and ES8311 codec.
- GPIO assignments follow the approved spec: display GPIO3/7/15/16/17/18, codec GPIO4/5/6/11/12/13/14, speaker gate GPIO9, and button GPIO42.
- Never drive GPIO10 board power control.
- Never format, erase, or silently migrate the MicroPython data partition; verify its offset, size, format, and mount behavior first.
- Do not add a production library or runtime dependency until the user approves the concrete dependency list and versions.
- Local files use matching MJPEG and MP3 base names; HTTP(S) entries must serve compatible media directly. YouTube watch pages and SMB/NFS shares require an HTTP(S) relay.
- Keep network controls on the local network; do not add telemetry, cloud accounts, or public internet administration.
- Dashboard controls must be keyboard operable, programmatically labeled, visibly focused, adequately contrasted, and expose status in text as well as color.
- Do not claim display/audio operation, partition preservation, Section 508 conformance, or production readiness without the corresponding evidence.

---

## File Map

- `firmware/MiniTV/MiniTV.ino`: Arduino application setup and task startup only.
- `firmware/MiniTV/aipi_lite_profile.h`: fixed AIPI Lite pin, screen, and audio constants.
- `firmware/MiniTV/display.{h,cpp}`: ST7735 initialization, status rendering, and brightness control.
- `firmware/MiniTV/button.{h,cpp}`: GPIO42 debouncing and short/double/long press events.
- `firmware/MiniTV/storage.{h,cpp}`: partition discovery, read-only mount, then guarded filesystem access after compatibility proof.
- `firmware/MiniTV/media_catalog.{h,cpp}`: discover and validate local channel/video/audio pairs.
- `firmware/MiniTV/media_source.{h,cpp}`: common bounded read interface for file-backed and HTTP(S)-backed resources.
- `firmware/MiniTV/audio_codec.{h,cpp}`: ES8311 control-bus initialization, I2S output, amplifier gate, mute, and volume.
- `firmware/MiniTV/playback.{h,cpp}`: channel selection, video/audio task coordination, and visible playback/error state.
- `firmware/MiniTV/playlist.{h,cpp}`: versioned JSON schema validation and safe path/URL checks.
- `firmware/MiniTV/network.{h,cpp}`: Wi-Fi connection and HTTP(S) source transport.
- `firmware/MiniTV/web_ui.h`: embedded dashboard HTML, CSS, and JavaScript.
- `firmware/MiniTV/web_server.{h,cpp}`: local control endpoints, upload handling, and diagnostics.
- `tests/host/`: host-side tests for playlist, channel, source, and playback policy logic.
- `tests/test_web_ui_accessibility.py`: built-in `unittest` checks for embedded HTML accessibility requirements.
- `README.md`: build, media preparation, first-run, playlist, and supported-source workflow.
- `docs/DEPENDENCIES.md`: candidate libraries, exact versions, licenses, and user approval record.
- `docs/COMPATIBILITY.md`: verified MicroPython partition evidence and native mount result.
- `docs/RECOVERY.md`: backup, USB recovery, and restoration procedure.

## Tasks

### Task 1: Establish the current firmware and data-partition facts

**Files:**
- Create: `docs/COMPATIBILITY.md`

**Interfaces:**
- Produces: a record of the MicroPython build/version, partition table layout, filesystem type, filesystem start/size, and evidence source; later storage code must use these recorded values and no guessed offset.

- [x] Inspect the AIPI Lite firmware installation instructions and available MicroPython `ESP32_GENERIC_S3-SPIRAM_OCT` partition evidence without writing to the board.
- [x] Record which facts about the current filesystem and native mount are unknown, rather than infer them from a partition name.
- [x] Where local firmware sources do not establish these facts, document the needed read-only partition-table/flash inspection and stop before any device write.
- [x] Record `UNKNOWN` for hardware evidence that cannot be obtained without the physical device; do not infer mount compatibility from the partition name.
- [x] Commit the evidence record with `docs: record AIPI Lite filesystem compatibility`.

**Acceptance:** The storage format and bounds are known from authoritative build evidence or clearly marked unverified, and no board flash/filesystem write occurred.

### Task 2: Review and obtain approval for native production dependencies

**Files:**
- Create: `docs/DEPENDENCIES.md`

**Interfaces:**
- Produces: a dependency table naming package, version, purpose, license, and source; firmware setup may proceed only after user approval of each required new production dependency.

- [x] Record the exact proposed Arduino ESP32 core, graphics, JPEG, MP3, and JSON dependencies in `docs/DEPENDENCIES.md`; replace the upstream Helix candidate with minimp3 pending licensing/performance review.
- [x] Identify which capabilities come from the Arduino/ESP32 core and which require standalone libraries.
- [x] Record the source, license, target notes, and exact pin proposed for each library.
- [x] Add the exact proposed production list to `docs/DEPENDENCIES.md`; do not add package declarations, downloads, or vendored code yet.
- [x] Obtain user approval for the concrete dependency list before proceeding to dependency-adding work.

**Acceptance:** The user has approved the exact production dependencies and versions before any is added to a manifest or firmware build. User approval recorded 2026-09-30; no third-party code downloaded or vendored yet.

### Task 3: Create a reproducible ESP32-S3 build scaffold

**Files:**
- Create: `firmware/MiniTV/MiniTV.ino`
- Create: `firmware/MiniTV/aipi_lite_profile.h`
- Create: `firmware/MiniTV/build_config.md`
- Modify: `docs/DEPENDENCIES.md`

**Interfaces:**
- `aipi_lite_profile.h` exports `constexpr` constants for all approved display, button, and codec pins and a `kProfileName` string equal to `"AIPI_LITE_XY006PL01"`.
- `MiniTV.ino` provides `setup()` and `loop()` and must not initialize GPIO10.

- [x] Add Arduino IDE setup notes and a provisional ESP32-S3 target configuration. Partition selection remains blocked on the actual device's filesystem evidence.
- [x] Record the approved exact library versions in the build notes; no package manifest applies to this Arduino IDE sketch and no third-party source has been downloaded or vendored.
- [x] Add `aipi_lite_profile.h` with the display, codec, speaker, and button constants from the spec.
- [x] Add a safe boot in `setup()` that initializes serial diagnostics only and keeps GPIO10 untouched; keep `loop()` idle until later tasks add components.
- [ ] Build the empty ESP32-S3 sketch using the pinned toolchain and confirm the selected partition CSV is the verified one; commit as `build: scaffold AI-PI Lite MiniTV firmware`. Blocked: no Arduino build tool is installed, and the device partition map remains unknown.

**Acceptance:** The source builds for ESP32-S3 without a CYD profile, uses the verified partition CSV, and does not contain GPIO10 writes.

### Task 4: Implement and test the AIPI Lite display and button layer

**Files:**
- Create: `firmware/MiniTV/display.h`
- Create: `firmware/MiniTV/display.cpp`
- Create: `firmware/MiniTV/button.h`
- Create: `firmware/MiniTV/button.cpp`
- Create: `tests/host/test_button.cpp`
- Modify: `firmware/MiniTV/MiniTV.ino`

**Interfaces:**
- `Display::begin() -> bool`, `Display::showStatus(const char* title, const char* detail) -> void`, and `Display::setBrightness(uint8_t percent) -> void`.
- `Button::update(uint32_t nowMs) -> ButtonEvent`, where `ButtonEvent` is `None`, `ShortPress`, `DoublePress`, or `LongPress`.

- [ ] Write host tests for active-low press, contact bounce, single press, double press, long press, and wrap-safe millisecond rollover; run them and confirm the new cases fail before implementation.
- [x] Implement the button state machine without blocking delays and with constants for debounce and click-window timing in `button.h`.
- [x] Initialize the 128 x 128 display on GPIO15/16/17/7/18 and backlight GPIO3, rotation 1, RGB color order, with explicit text labels for status and errors.
- [x] Add the display and button to `setup()`/`loop()`; drive only GPIO3 and GPIO42 as documented.
- [ ] Run host button tests and target firmware build. Tests were not added or run; the Arduino build tool is unavailable.
- [x] Commit implementation as `feat: add AIPI Lite display and button profile`.

**Acceptance:** Host button tests pass and the target build succeeds; physical orientation/contrast remains marked unverified until observed on device.

### Task 5: Mount the MicroPython data filesystem without risking its contents

**Files:**
- Create: `firmware/MiniTV/storage.h`
- Create: `firmware/MiniTV/storage.cpp`
- Create: `tests/host/test_storage_policy.cpp`
- Modify: `docs/COMPATIBILITY.md`
- Modify: `firmware/MiniTV/MiniTV.ino`

**Interfaces:**
- `Storage::beginReadOnly(const PartitionBounds& expected) -> StorageResult` validates the partition identity, bounds, and filesystem before mounting.
- `Storage::beginWritableAfterCompatibilityProof() -> StorageResult` is inaccessible until a successful read-only compatibility check is recorded.
- `Storage::fileSystem() -> fs::FS&` is available only after successful mount.

- [ ] Write host tests proving mismatched offset, size, type, or mount error returns failure and never calls format or write operations.
- [ ] Run the tests and confirm they fail before implementing the policy.
- [ ] Implement partition lookup from the Task 1 evidence; reject missing, ambiguous, or out-of-bounds partition records.
- [ ] Attempt read-only mount first and render/report the result; never call `format()`, `erase_range()`, or an implicit format-on-mount option.
- [ ] Enable write calls only after a physical read-only mount confirms the MicroPython files remain readable; record the hardware evidence in `docs/COMPATIBILITY.md` and commit as `feat: safely mount the AIPI Lite media filesystem`.

**Acceptance:** Host safety tests pass; source has no automatic format/erase path; writable mounting remains disabled until physical compatibility evidence is recorded.

### Task 6: Add local media catalog and synchronized MJPEG/MP3 playback

**Files:**
- Create: `firmware/MiniTV/media_catalog.h`
- Create: `firmware/MiniTV/media_catalog.cpp`
- Create: `firmware/MiniTV/media_source.h`
- Create: `firmware/MiniTV/media_source.cpp`
- Create: `firmware/MiniTV/audio_codec.h`
- Create: `firmware/MiniTV/audio_codec.cpp`
- Create: `firmware/MiniTV/playback.h`
- Create: `firmware/MiniTV/playback.cpp`
- Create: `tests/host/test_media_catalog.cpp`
- Modify: `firmware/MiniTV/MiniTV.ino`
- Modify: `README.md`
- Modify: `docs/RECOVERY.md`

**Interfaces:**
- `MediaPair` contains `String title`, `String videoPath`, `String audioPath`, and `uint32_t durationMs` (zero when unknown).
- `MediaCatalog::scan(fs::FS& filesystem, CatalogResult& result) -> bool` discovers `/Videos/<number>/` and optional `/Videos/random/` pairs.
- `MediaSource::open(const char* path) -> bool`, `read(uint8_t* output, size_t capacity) -> int`, `available() -> int`, and `close() -> void` provide bounded file reads.
- `AudioCodec::begin(uint32_t sampleRate) -> bool`, `setMuted(bool) -> void`, and `setVolume(uint8_t percent) -> void` control ES8311 output.
- `PlaybackController::begin() -> bool`, `next() -> void`, `previous() -> void`, `setMuted(bool) -> void`, and `tick() -> void` drive playback.

- [ ] Write media catalog tests for matching pairs, missing audio/video, duplicate names, invalid paths, empty channel folders, and random selection; run them and confirm they fail before implementation.
- [ ] Implement local catalog scanning under `/Videos/`; accept only normalized paths below `/Videos/` and reject traversal components.
- [ ] Implement `MediaSource` over the verified mounted filesystem using a fixed bounded read buffer; close both media streams on every failure path.
- [ ] Configure GPIO4/5 ES8311 control and GPIO6/11/12/13/14 I2S from the existing AIPI Lite register evidence; keep GPIO9 amplifier disabled until codec init succeeds and disable it on every exit.
- [ ] Decode MJPEG frames to the 128 x 128 display and MP3 audio through ES8311, keeping decode/read buffers in PSRAM where supported; use GPIO42 short/double/long events for channel navigation and mute.
- [ ] Run host catalog tests and firmware build; on physical hardware verify one prepared local MJPEG/MP3 pair, visible error behavior, and audible output before marking playback complete.
- [ ] Document the exact media conversion commands, folder structure, and manual recovery path in `README.md` and `docs/RECOVERY.md`; commit as `feat: play local media on AI-PI Lite`.

**Acceptance:** Host tests pass and the firmware build succeeds. Local playback is considered hardware-verified only after the device displays frames and produces audible output without filesystem damage.

### Task 7: Add a versioned playlist and bounded HTTP(S) media sources

**Files:**
- Create: `firmware/MiniTV/playlist.h`
- Create: `firmware/MiniTV/playlist.cpp`
- Create: `firmware/MiniTV/network.h`
- Create: `firmware/MiniTV/network.cpp`
- Create: `tests/host/test_playlist.cpp`
- Modify: `firmware/MiniTV/media_source.h`
- Modify: `firmware/MiniTV/media_source.cpp`
- Modify: `README.md`

**Interfaces:**
- `Playlist::load(fs::FS& filesystem, const char* path, PlaylistResult& result) -> bool` parses the versioned document.
- `PlaylistEntry` has `id`, `title`, `sourceKind`, `videoLocation`, and `audioLocation` fields; `sourceKind` is exactly `"file"` or `"http"`.
- `HttpMediaSource::open(const char* url) -> bool`, `read(uint8_t* output, size_t capacity) -> int`, and `close() -> void` use the same source contract as local files.

- [ ] Define and document version 1 JSON with root keys `version` and `channels`; each channel has `id`, `title`, `source`, `video`, and `audio`.
- [ ] Add test fixtures for valid local/network entries, invalid JSON, wrong version, missing fields, duplicate IDs, oversized input, unsupported schemes, traversal paths, and unsupported file extensions; confirm failures before implementing the parser.
- [ ] Implement bounded parsing with exact input/entry/string limits and reject anything outside `/Videos/` for local media or `http://`/`https://` URLs for network media.
- [ ] Implement HTTP(S) reads with redirect limits, timeouts, response-size caps, content-type checks, stream cancellation, and diagnostics; never disable TLS certificate validation.
- [ ] Connect network loss and decode failures to explicit playback status and retry only after a fresh user playback request.
- [ ] Run host playlist tests and build; document direct media URL requirements and the PC relay requirement for YouTube/SMB/NFS sources; commit as `feat: stream playlist media over Wi-Fi`.

**Acceptance:** Host playlist tests pass, streaming reads remain bounded, unsupported pages/shares fail visibly, and no credentials or signed URLs appear in tracked fixtures/logs.

### Task 8: Add local web controls and accessible library management

**Files:**
- Create: `firmware/MiniTV/web_ui.h`
- Create: `firmware/MiniTV/web_server.h`
- Create: `firmware/MiniTV/web_server.cpp`
- Create: `tests/test_web_ui_accessibility.py`
- Create: `tests/test_web_server_policy.cpp`
- Modify: `firmware/MiniTV/MiniTV.ino`
- Modify: `README.md`

**Interfaces:**
- `WebServer::begin() -> bool` starts only on the configured local Wi-Fi interface.
- `POST /api/playback` accepts a validated channel ID and action from `next`, `previous`, `mute`, `volume`, `brightness`, `sleep`, or `restart`.
- `GET /api/channels` returns channel IDs, titles, order, and media presence without returning signed URLs.
- `PUT /api/channels/{id}` accepts a validated title/order update; `DELETE /api/channels/{id}` removes the channel manifest entry and only deletes media after an explicit UI confirmation.
- `GET /api/status` returns channel/title, playback state, mute/volume/brightness, Wi-Fi status, and sanitized error codes; it never returns passwords, tokens, signed URLs, or raw request headers.
- `POST /api/upload` accepts a bounded MJPEG/MP3 upload only when writable filesystem compatibility has been proven.

- [ ] Write API policy tests for unknown action, invalid channel, out-of-range volume/brightness, oversized upload, path traversal, and secret redaction; confirm they fail before server implementation.
- [ ] Build semantic HTML with labeled controls, keyboard navigation, visible focus, contrast-safe styles, and `aria-live` text updates; do not use color as the only state signal.
- [ ] Add Python standard-library accessibility tests with docstrings on every generated Python method; check form labels, button names, focus styles, and text status in `web_ui.h`.
- [ ] Implement local playback/status, channel list/rename/reorder/delete, brightness/volume, sleep/restart routes, and bounded uploads; channel deletion requires explicit confirmation and must not delete files until the confirmation action is received.
- [ ] Save playlist/settings changes with a temporary file, flush/close, and atomic rename; preserve the prior file if a write or rename fails.
- [ ] Run the C++ policy tests, Python accessibility tests, and target build; manually inspect the interface at mobile and desktop widths before commit `feat: add accessible local MiniTV controls`.

**Acceptance:** All host tests pass; browser controls are keyboard-operable and expose status text; the interface binds only to the LAN and does not disclose secrets.

### Task 9: Add Wi-Fi setup, diagnostics, and recovery-safe maintenance

**Files:**
- Create: `firmware/MiniTV/diagnostics.h`
- Create: `firmware/MiniTV/diagnostics.cpp`
- Create: `firmware/MiniTV/update.h`
- Create: `firmware/MiniTV/update.cpp`
- Modify: `firmware/MiniTV/web_server.cpp`
- Modify: `docs/RECOVERY.md`
- Modify: `README.md`

**Interfaces:**
- `Diagnostics::snapshot() -> DiagnosticSnapshot` returns sanitized version, free memory, filesystem bounds/free space, Wi-Fi state, and last error code.
- `Update::validateImage(Stream& image, size_t length) -> bool` validates declared size and image metadata before writing an inactive OTA slot.
- `Update::apply(Stream& image, size_t length) -> UpdateResult` is enabled only after an A/B partition layout and rollback path are physically verified.

- [ ] Add first-run Wi-Fi setup and editable local Wi-Fi settings without hard-coded credentials; store credentials in protected device settings and redact them from status, logs, and exports.
- [ ] Add diagnostics for playback errors, heap/PSRAM, Wi-Fi state, and filesystem capacity without exposing secrets or raw signed URLs.
- [ ] Verify the existing and proposed partition maps leave room for a safe OTA slot without moving the MicroPython data filesystem; if not verified, leave OTA disabled.
- [ ] Implement OTA only for a verified inactive app slot, with image size/type validation, interrupted-update rollback, and no filesystem writes during app update.
- [ ] Run policy tests for redaction and image rejection; document USB recovery, original firmware restoration, and backup steps before enabling OTA; commit as `feat: add safe maintenance and recovery tools`.

**Acceptance:** Diagnostics redact secrets; OTA stays unavailable unless the update/rollback layout has physical evidence and documented recovery; a failed update cannot format or overwrite media storage.

## Execution Notes

- Run tests before the implementation step they cover, and rerun the complete host suite after each task that changes shared interfaces.
- Use `git diff --check` and inspect the complete diff before each commit.
- Do not flash physical firmware as part of implementation unless the user separately requests it; report build success separately from hardware evidence.
- The only permitted execution mode in this workspace is inline; do not delegate tasks to subagents.
- Stop at the dependency approval step until the user authorizes the exact production packages and versions.
