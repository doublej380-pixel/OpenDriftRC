# [OpenDrift EdgeTX tool](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.1/OpenDrift.lua)

This tool supports both Open Beta **full-duplex** CRSF firmware targets:
`waveshare_amoled_164_crsf` and `waveshare_128_crsf`.

Download [`OpenDrift.lua`](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.1/OpenDrift.lua), copy it to `SCRIPTS/TOOLS/OpenDrift.lua` on the radio SD card,
then launch **OpenDrift** from the [EdgeTX Tools menu](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.1/OpenDrift.lua).

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
Speed, Prediction, Servo Quiet, Steering Travel, Servo Travel, Servo Center,
Servo Reverse, and Gyro Reverse.
