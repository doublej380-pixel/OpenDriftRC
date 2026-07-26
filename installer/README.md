# OpenDrift browser installer

This folder is a static ESP Web Tools installer for the two supported OpenDrift
ESP32-S3 display boards.

- `manifest-amoled.json` installs the primary Waveshare AMOLED 1.64 build.
- `manifest-round.json` installs the deprecated Waveshare round 1.28 build.
- `firmware/` contains merged factory images flashed at offset `0x0000`.

The installer must be served over HTTPS for Web Serial access. Opening
`index.html` directly from disk is sufficient for visual review, but not for
flashing hardware.

The public account-free installer is deployed through GitHub Pages at:

`https://doublej380-pixel.github.io/OpenDrift/`

Firmware flashing runs through the browser's local Web Serial connection. The
site has no sign-in, analytics, or firmware-upload service.

Factory images include the bootloader, partition table, OTA boot data, and
application. Do not replace them with PlatformIO's application-only
`firmware.bin`.

`site-host/` is the zero-dependency static build used by the connected
production host. For that deployment, copy the static installer into
`site-host/public/`, rename `index.html` to `installer.html`, and deploy with
`site-host/` as the source root with the project's `.openai/hosting.json`. Its
build script packages the installer under `dist/client`, adds the asset-serving
entrypoint, and copies the Sites project metadata into `dist`. GitHub Pages
continues to publish this folder directly.
