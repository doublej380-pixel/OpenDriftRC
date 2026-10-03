# USB maintenance SD readiness

The screen's SD-ready indication means the card is mounted on the ESP and
its read-only USB LUN has been initialized. It does not mean that the host
has finished discovering the disk or mounting its FAT filesystem.

## Documentation comparison

The [ESP32-S3 datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)
describes a full-speed USB OTG peripheral. This is not a high-speed USB disk.
The [Espressif USB device guide](https://docs.espressif.com/projects/esp-usb/en/latest/esp32s3/usb_device.html#msc-performance-optimization)
describes MSC buffer-size/performance tradeoffs. Its modern esp_tinyusb APIs
are not drop-in replacements for our Arduino core's older USBMSC implementation.

Our installed Arduino framework already has a 4096-byte MSC buffer, but
OpenDrift used to return only 512 bytes per callback, hand each sector to a
worker, and poll completion with `delay(1)`. This adds callback/scheduler overhead
while the host scans FAT/directory sectors. That is a plausible contributor to
slow mounting, not proof of the complete cause of a reported one-minute delay.

USB SD reads now fill up to 4096 bytes per worker request and signal completion
using a binary semaphore. The 250 ms cooperative-wait limit and TinyUSB busy
retry path remain. The actual SD reads still use individual CMD17 transactions,
avoiding the earlier problematic CMD18 multi-sector path. No card format,
partition, SPI frequency, USB descriptor/LUN order, or firmware-updater behavior
is changed. The SD drive remains read-only; normal driving/logging is unchanged.

## Hardware validation

1. Park the car; disconnect servo/ESC power before updating the test firmware.
2. Enter USB maintenance with the same SD card and computer used previously.
3. Time USB-mode entry to disk appearance, then clicking the SD drive to its
   directory becoming usable. Repeat at least three times, including one cold
   power-up. Host caching can make subsequent attempts faster.
4. Copy an existing CSV to the computer and compare it byte-for-byte against
   a copy taken with a normal SD reader. Do not format or repair the card for
   this test; that would change the comparison and can destroy logs.
5. Check that the firmware-update drive remains accessible and the screen's
   maintenance restart still returns to normal mode.

If it still stalls, capture the maintenance serial output from immediately
after reconnecting USB until the directory opens. `USB SD reads=...` includes
callback sizes, SD I/O duration, callback wait, busy retries, errors, and worker
state. `USB SD activity:` adds uptime and average bytes-per-second since the
previous report (the first report also includes startup idle time).

- Growing byte/read counts without errors suggest an active host scan. Compare
  request sizes and throughput before/after, rather than assuming enumeration
  has failed.
- Busy returns, errors, invalid requests, or request mismatches need a closer
  card/transport investigation.
- A delay with no SD callbacks points toward host discovery/probing or mounting,
  but is not sufficient to identify a specific host or USB-stack fault.

On Linux, `journalctl -k -f` during connection can reveal resets/SCSI timeouts.
Use `lsblk -f` to see when the disk and filesystem are discovered. Do not run
repair or formatting commands as part of the diagnostic capture.
