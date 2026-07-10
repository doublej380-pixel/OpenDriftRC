# OpenDrift

OpenDrift is an open source drift gyro for RC drift cars, built around the Waveshare ESP32-S3 round touch display board. It reads steering and gain channels from a receiver, mixes driver steering with gyro correction, outputs a servo signal, and exposes tuning through both the onboard touch UI and a WiFi web configurator.

The goal is to make gyro setup less of a black box: you can see receiver signals, calibrate steering, reverse servo or gyro direction independently, and tune the drift behavior at the car.

## Current Features

- ESP32-S3 firmware using PlatformIO and Arduino.
- Round touch display UI.
- IMU yaw-rate based gyro correction.
- Receiver steering input.
- Receiver gyro gain input.
- Servo output with center, reverse, and travel settings.
- Steering calibration for max left, center, and max right.
- Separate servo reverse and gyro reverse.
- Gyro tuning:
  - gain
  - deadband
  - maximum correction
  - smoothing
  - steering cut
- Do-nothing signal-loss behavior when steering input is lost.
- WiFi access point.
- Browser-based web configurator.
- Persistent settings stored in ESP32 preferences.

## Hardware Routing

Current default pinout:

| Signal | GPIO | Direction | Notes |
| --- | ---: | --- | --- |
| Servo output | 16 | Output | Goes to steering servo signal |
| Receiver steering | 17 | Input | Standard RC PWM input |
| Receiver gyro gain | 18 | Input | Standard RC PWM input from knob/aux channel |

Make sure the receiver, ESP32 board, and servo power system share ground.

The servo should be powered from a suitable BEC or ESC receiver rail. Do not rely on the ESP32 board to power a steering servo.

## Build And Upload

The firmware project is in:

`OpenDrift/`

Build with PlatformIO:

```sh
pio run
```

Upload with your normal PlatformIO upload workflow for the `waveshare_128` environment.

Main dependencies are managed in `OpenDrift/platformio.ini`:

- SensorLib
- LovyanGFX
- ESP32Servo
- CST816S

## First Power-On

On boot, OpenDrift initializes:

1. Display
2. Settings
3. IMU
4. Servo output
5. Receiver inputs
6. Touch
7. Gyro calibration
8. WiFi, if enabled
9. UI

Keep the car still during startup so gyro calibration can capture a clean yaw-rate offset.

## Onboard UI Pages

Swipe left/right to move between pages.

### Main

Shows the current gyro gain and provides:

- `- / +`: adjust stored gain when radio gain input is not overriding it.
- `CAL`: recalibrate gyro offset. Use this when the car is sitting still and the gyro seems biased.

### Gyro

Basic gyro behavior settings:

- `Deadband`: yaw-rate zone ignored around zero.
- `GYRO REV`: reverses only the gyro correction direction.

Use `GYRO REV` when normal steering direction is correct, but the gyro counter-steers the wrong way.

### Tune

Trackside tuning page for the current algorithm:

- `MAX CORR`: maximum gyro correction in servo microseconds.
- `SMOOTH`: low-pass filter amount for yaw-rate response.
- `STEER CUT`: reduces gyro authority as driver steering increases.

Good starting values:

| Setting | Start |
| --- | ---: |
| Deadband | 3.0 |
| Max correction | 180 |
| Smoothing | 0.08 |
| Steering cut | 0.60 |

Tune symptoms:

| Symptom | Try |
| --- | --- |
| Weaves left/right driving straight | Increase deadband, lower gain, or lower smoothing |
| Spins out easily | Lower max correction first |
| Won't hold drift | Raise max correction slowly |
| Feels like it fights steering input | Increase steering cut |
| Feels slow or lazy | Increase smoothing slightly or raise gain |
| Feels twitchy | Lower smoothing or gain |

### Radio

Live receiver monitor:

- Steering pulse and signal state.
- Gain pulse and signal state.
- Current gain value.
- Calibration values.
- Position bars for steering and gain.

Use this page to confirm the receiver is connected and moving through a sensible range.

### Steering

Steering calibration and servo direction:

- `MAX LEFT`: capture current steering pulse as full left.
- `CENTER`: capture current steering pulse as neutral.
- `MAX RIGHT`: capture current steering pulse as full right.
- `REV`: reverse physical servo direction.

Suggested calibration flow:

1. Hold steering full left and tap `MAX LEFT`.
2. Release steering to neutral and tap `CENTER`.
3. Hold steering full right and tap `MAX RIGHT`.
4. Check that the output value centers around `1500`.
5. Use `REV` only if normal steering direction is backwards.

Servo reverse and gyro reverse are separate on purpose:

- Use `REV` on the Steering page when driver steering moves the wheels backward.
- Use `GYRO REV` on the Gyro page when driver steering is correct but gyro correction is backward.

### WiFi

Shows WiFi state and connected client count.

- `WIFI ON/OFF`: toggles the access point.

When WiFi is enabled, connect to the `OpenDrift` network and open:

`http://192.168.4.1/`

### System

Basic firmware/system information.

## Web Configurator

When WiFi is enabled, OpenDrift starts a web configurator at:

`http://192.168.4.1/`

Current web settings:

- Gyro gain
- Deadband
- Reverse gyro correction
- Max correction
- Smoothing
- Steering cut
- Servo reverse
- Servo center
- Servo travel
- Steering max left / center / max right
- Gain channel low / high
- WiFi enabled on boot
- WiFi auto-off timeout

The web page also shows live receiver pulse values for steering and gain.

Use the web configurator when you want to make several changes quickly. Use the onboard UI when tuning trackside without a phone or laptop.

## Gyro Algorithm Overview

The current control loop works like this:

1. Read IMU yaw rate.
2. Subtract calibrated gyro offset.
3. Apply deadband.
4. Smooth yaw rate using a tunable low-pass filter.
5. Convert yaw rate into correction using gyro gain.
6. Limit correction with max correction.
7. Optionally reverse gyro correction.
8. Reduce gyro authority based on steering input using steering cut.
9. Add gyro correction to calibrated steering command.
10. Apply servo center/reverse/travel and output to the servo.

This keeps driver steering as the base command and lets the gyro assist rather than fully take over.

## Signal Loss Behavior

If steering receiver input is missing, OpenDrift does not send a new servo command.

This leaves the servo at the last commanded position. It is intentionally a simple do-nothing behavior while failsafe strategy is still being evaluated.

## Tuning Notes

Start with low correction and work upward:

1. Confirm steering calibration.
2. Confirm servo direction.
3. Confirm gyro direction.
4. Set gain low.
5. Set max correction around `180`.
6. Set smoothing around `0.08`.
7. Set steering cut around `0.60`.
8. Drive straight and remove weave first.
9. Then tune drift hold.

For straight-line weave, do not start by increasing max correction. Weave usually means too much sensitivity near center.

For spin-outs, lower max correction before changing everything else.

For poor drift hold, increase max correction in small steps after straight-line behavior is calm.

## Project Layout

Important folders:

- `OpenDrift/src/main.cpp`: main firmware loop and control mixing.
- `OpenDrift/lib/GyroController`: gyro filtering and correction generation.
- `OpenDrift/lib/RadioInput`: receiver PWM input capture.
- `OpenDrift/lib/Servo`: servo output wrapper.
- `OpenDrift/lib/Settings`: persistent settings.
- `OpenDrift/lib/UI`: onboard touch UI.
- `OpenDrift/lib/WebConfigurator`: web settings page.
- `OpenDrift/lib/WIFIManager`: WiFi access point control.

## License

See `LICENSE`.
