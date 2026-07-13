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
- Separate radio steering travel limit.
- Separate servo reverse and gyro reverse.
- Gyro tuning:
  - gain
  - deadband
  - maximum correction
  - smoothing
  - attack speed
  - return speed
- Do-nothing signal-loss behavior when steering input is lost.
- WiFi access point.
- Browser-based web configurator.
- Optional onboard blackbox logging to flash.
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

### Gyro Tune

Trackside tuning page for correction strength and filtering:

- `MAX CORR`: maximum gyro correction in servo microseconds.
- `SMOOTH`: time-based low-pass filter amount. Higher values are smoother/slower.
- `I GAIN`: integral correction strength. `0.00` is off.
- `I LIM`: maximum integral correction in servo microseconds.
- `HOLD`: extra high-yaw correction boost. `0` is off; higher values add more authority once the car is already rotating and settled.

`SMOOTH` is intentionally inverted from raw filter math: higher numbers mean more smoothing and slower gyro response.

Deadband is applied as a soft deadband. Small yaw noise is still ignored, but correction fades in from zero instead of jumping as soon as yaw crosses the deadband value.

### Response

Trackside tuning page for how quickly steering and gyro correction move:

- `ATTACK`: how quickly gyro correction can build, in fine 1-step adjustments.
- `RETURN`: how quickly gyro correction can return toward center, in fine 1-step adjustments.
- `DAMPER`: steering input damping in milliseconds. `0` is off; higher values calm faster driver steering changes.
- `WOBBLE`: gyro correction anti-wobble strength. `0` is off, `50` is the current baseline, `100` is strongest.

Good starting values:

| Setting | Start |
| --- | ---: |
| Deadband | 3.0 |
| Max correction | 480 |
| Smoothing | 0.90 |
| I gain | 0.00 |
| I limit | 120 |
| Hold | 0 |
| Attack | 80 |
| Return | 30 |
| Damper | 0 |
| Wobble | 50 |

Tune symptoms:

| Symptom | Try |
| --- | --- |
| Weaves left/right driving straight | Increase deadband, lower gain, or raise smoothing |
| Spins out easily | Lower max correction or return speed first |
| Won't hold drift | Raise max correction slowly |
| Holds long drifts but entry/exit feels vague | Add a small amount of I gain |
| Exits feel like correction hangs on too long | Lower I gain or I limit |
| Transitions snap back too hard | Lower return speed |
| Steering input feels too abrupt | Increase damper |
| Countersteer arrives too slowly | Increase attack speed |
| Feels slow or lazy | Lower smoothing slightly or raise gain |
| Feels twitchy | Raise smoothing or lower gain |

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
- `TRV`: limits the final steering command range for driver input and gyro correction.
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

Steering calibration and steering travel are separate on purpose:

- Use `MAX LEFT`, `CENTER`, and `MAX RIGHT` to teach OpenDrift what the receiver outputs.
- Use `TRV` to limit how far the mixed steering command is allowed to move.
- Use servo travel when you need to scale the final physical servo output, including gyro correction.

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
- I gain
- I limit
- Hold boost
- Attack speed
- Return speed
- Steering input damper
- Anti-wobble strength
- Servo reverse
- Servo center
- Servo travel
- Steering max left / center / max right
- Radio steering travel
- Gain channel low / high
- WiFi enabled on boot
- WiFi auto-off timeout
- Blackbox logging enabled

The web page also shows live receiver pulse values for steering and gain.

Use the web configurator when you want to make several changes quickly. Use the onboard UI when tuning trackside without a phone or laptop.

## Onboard Blackbox Log

OpenDrift can store CSV-style blackbox logs in onboard FFat flash storage at about 20 Hz while steering receiver signal is present.

Blackbox logging is disabled by default. Enable `Onboard logging` in the web configurator only when you want to collect data, then save settings.

To avoid disturbing gyro timing, log rows are buffered in RAM while driving. The control loop never auto-flushes the log to flash. Use the web configurator to flush, download, or clear the log after the run.

The current firmware uses a 16 MB flash partition layout based on the Chronos Navio Waveshare S3 1.28 configuration. After changing to this layout, do a full flash erase once before uploading if the board bootloops or the log storage acts strange.

The web configurator shows the current log size and provides:

- `Download CSV`
- `Flush Log`
- `Clear Log`

Log rows include:

- Time
- Raw yaw rate
- Filtered yaw rate
- Raw gyro correction
- Slewed gyro correction
- Steering input and calibrated steering command
- Servo output
- Gain input and active gain
- Active deadband, max correction, smoothing, hold boost, anti-wobble, attack, and return
- Active I gain, I limit, and I correction
- Steering/gain signal state

Suggested test workflow:

1. Connect to the `OpenDrift` WiFi network.
2. Open `http://192.168.4.1/`.
3. Enable onboard logging if it is off, then save settings.
4. Tap `Clear Log`.
5. Drive the car.
6. Reconnect to WiFi and tap `Flush Log`, or use `Download CSV` which flushes first.
7. Download `opendrift-blackbox.csv`.

The CSV can be pasted into a spreadsheet or plotted to see whether the car spun because of delayed correction, overcorrection, max correction saturation, noisy yaw, or steering/radio behavior.

## Gyro Algorithm Overview

The current control loop works like this:

1. Read IMU yaw rate.
2. Subtract calibrated gyro offset.
3. Apply soft deadband.
4. Smooth yaw rate using a time-based tunable low-pass filter.
5. Convert yaw rate into correction using gyro gain, with optional high-yaw hold boost.
6. Add a leaky, clamped integral correction when I gain is enabled.
7. Limit correction with max correction.
8. Stabilize tiny correction changes so servo-scale jitter does not become steering twitch.
9. Optionally reverse gyro correction.
10. Slew-limit correction using time-based attack and return speed.
11. Add gyro correction to calibrated steering command.
12. Apply servo center/reverse/travel and output to the servo.

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
5. Set max correction around `480`.
6. Set smoothing around `0.90`.
7. Leave I gain at `0.00` until the car is stable.
8. Set attack around `80`.
9. Set return around `30`.
10. Leave steering damper at `0` until the car is stable.
11. Drive straight and remove weave first.
12. Then tune drift hold.

For straight-line weave, do not start by increasing max correction. Weave usually means too much sensitivity near center.

For spin-outs, lower max correction or return speed before changing everything else.

For twitchy entries or driver-input snap, add steering damper in small steps. It smooths the radio steering command before gyro correction is added, so it should be used lightly.

For the new I term, start tiny. Try `I GAIN` around `0.05` to `0.20` with `I LIM` around `80` to `120`. If the car feels like it keeps steering after the rotation has settled, reduce I first.

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
