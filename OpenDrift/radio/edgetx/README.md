# OpenDrift EdgeTX tool

This tool is for the experimental **full-duplex** CRSF firmware. The current
bench-validated target is `waveshare_128_crsf`; the AMOLED CRSF target remains
RX-only until replacement hardware is validated.

Copy `SCRIPTS/TOOLS/OpenDrift.lua` to the same path on the radio SD card,
then launch **OpenDrift** from the EdgeTX Tools menu.

CRSF wiring for the full-duplex firmware:

- Receiver TX to OpenDrift GPIO 17
- Receiver RX to OpenDrift GPIO 18
- Receiver and OpenDrift grounds connected

Use the roller to select a setting, press it to enter edit mode, rotate to
change the value, and press again to finish. Changes are applied live and are
saved by OpenDrift's normal delayed settings writer. A successful radio write
also requests an immediate refresh of the current OpenDrift display page.

`Saved Gain` is the stored fallback gain. While CRSF channel 3 is assigned and
valid, channel 3 remains the active live gain control.

The tool exposes sixteen values: Saved Gain, Deadband, Max Correction,
Smoothing, Drift Memory, Memory Limit, Hold Assist, Countersteer, Tail Slide
Speed, Anti Wobble, Hunt Damping, Attack, Return, Input Damping, Terrain
Assist, and Gyro Reverse.
