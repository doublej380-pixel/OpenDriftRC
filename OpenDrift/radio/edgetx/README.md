# [OpenDrift EdgeTX tool](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.4/OpenDrift.lua)

This tool supports the AMOLED V1 and V2 **full-duplex** CRSF firmware targets:
`waveshare_amoled_164_crsf` and `waveshare_amoled_164_v2_crsf`.

Download [`OpenDrift.lua`](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.4/OpenDrift.lua), copy it to `SCRIPTS/TOOLS/OpenDrift.lua` on the radio SD card,
then launch **OpenDrift** from the [EdgeTX Tools menu](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.4/OpenDrift.lua).

CRSF wiring for the full-duplex firmware:

- Receiver TX to OpenDrift GPIO 17
- Receiver RX to OpenDrift GPIO 18
- Receiver and OpenDrift grounds connected

Use the roller to select a setting, press it to enter edit mode, rotate to
change the value, and press again to finish. Changes are applied live and are
saved by OpenDrift's normal delayed settings writer. A successful radio write
also requests an immediate refresh of the current OpenDrift display page.

`Active Gain` follows CRSF channel 3 live. The tool shows a reminder that
channel 3 overrides gain changes made elsewhere while its signal is valid; the
stored profile gain remains the fallback used without that gain signal.

The tool exposes the sixteen gyro and steering values: Active Gain, Deadband, Max Correction,
Smoothing, Drift Memory, Memory Limit, Hold Assist, Countersteer, Tail Slide
Speed, Prediction, Servo Quiet, Steering Travel, Servo Travel, Servo Center,
Servo Reverse, and Gyro Reverse. It also assigns CRSF channel 1–16 or OFF to
GPIO 1–8 on AMOLED V1 and GPIO 3–8 on AMOLED V2. GPIO 1/2 display `RES` on V2
because those pins carry the CRSF UART.
