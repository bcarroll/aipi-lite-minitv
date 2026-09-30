# AI-PI Lite MicroPython Filesystem Compatibility

**Status:** Pre-hardware review; filesystem identity and native mount compatibility are unverified.

## What repository evidence establishes

The adjacent AI-PI Lite firmware repository targets an ESP32-S3 with 16 MB flash
and 8 MB Octal PSRAM. Its installer selects the MicroPython
`ESP32_GENERIC_S3-SPIRAM_OCT` image. Its installer does not install or preserve
an explicit partition-table binary, and the checked-in files do not establish
the current device's partition offsets, sizes, filesystem type, or files.

MicroPython's ESP32 port can use FAT or LittleFS2 for its VFS. The ESP32 port
selects a `vfs` or fallback `ffat` partition at runtime; a partition label does
not establish the filesystem format. The actual device may also differ from a
fresh install because its history is unknown.

## Device-specific facts

| Fact | Result | Evidence |
| --- | --- | --- |
| MicroPython firmware family | `ESP32_GENERIC_S3-SPIRAM_OCT` is selected by the adjacent installer | Local adjacent repo installer in `../aipi-lite/` |
| Firmware build/version on this device | UNKNOWN | Requires read-only device inspection |
| Partition table and filesystem partition label | UNKNOWN | No exact partition table is installed by the inspected installer |
| Filesystem type (FAT or LittleFS2) | UNKNOWN | Must inspect the actual device; label is insufficient |
| Filesystem offset and size | UNKNOWN | Must inspect the actual device |
| Existing video paths and contents | UNKNOWN | Requires read-only listing from the actual mounted device |
| Native ESP-IDF/Arduino mount compatibility | UNKNOWN | No mount or preservation evidence exists |

## Required read-only inspection before storage implementation

1. Record the running MicroPython version/build and flash size without invoking
   an installer, erase, format, or firmware update.
2. Read the partition table from the device and record each partition's label,
   type/subtype, offset, and size. Identify the filesystem partition from the
   table and firmware's actual mount behavior, not by guessing a label.
3. Use the running MicroPython runtime to record `os.statvfs('/')`, list the
   media tree, and read a small existing file. Keep those outputs as evidence.
4. Establish whether the installed native framework can identify and mount
   that exact filesystem read-only without format-on-mount behavior. A
   successful partition lookup alone is not mount compatibility evidence.
5. Back up the partition before any future write test. A read-only native mount
   and file-content comparison must pass before firmware enables writes.

No board write, filesystem mount, format, erase, migration, or flash operation
was performed while producing this report. Until the evidence above exists,
firmware must not select a partition offset by assumption, mount writable, or
perform updates that could overwrite the MicroPython filesystem.

## References

- [MicroPython filesystem documentation](https://docs.micropython.org/en/v1.28.0/reference/filesystem.html)
- [MicroPython ESP32 `flashbdev.py`](https://github.com/micropython/micropython/blob/v1.28.0/ports/esp32/modules/flashbdev.py)
- [MicroPython ESP32 port README](https://github.com/micropython/micropython/blob/v1.28.0/ports/esp32/README.md)
- Adjacent AI-PI Lite firmware and installer: `../aipi-lite/`
