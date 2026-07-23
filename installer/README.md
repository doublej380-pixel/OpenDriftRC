# OpenDrift browser installer

This folder is a static ESP Web Tools installer for the two supported OpenDrift
ESP32-S3 display boards.

- `manifest-amoled.json` installs the primary Waveshare AMOLED 1.64 build.
- `manifest-round.json` installs the deprecated Waveshare round 1.28 build.
- `firmware/` contains merged factory images flashed at offset `0x0000`.

The installer must be served over HTTPS for Web Serial access. Opening
`index.html` directly from disk is sufficient for visual review, but not for
flashing hardware.

Factory images include the bootloader, partition table, OTA boot data, and
application. Do not replace them with PlatformIO's application-only
`firmware.bin`.
