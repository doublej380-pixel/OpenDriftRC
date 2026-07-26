# OpenDrift

OpenDrift is an open source drift gyro for RC drift cars, built around the Waveshare ESP32-S3 Touch AMOLED 1.64 board. It reads steering, throttle, and optional gain channels from a receiver, mixes driver steering with gyro correction, outputs steering and optional throttle signals, and exposes tuning through both the onboard touch UI and a WiFi web configurator.

OpenDrift v1.0 uses a dedicated 250 Hz control task, a single gyro filter, continuous yaw prediction, throttle-informed load-change prediction, and non-accumulating quiet-drift reference feedback. The goal is to make gyro setup less of a black box while keeping the active control path small enough to reason about.

See the current [Technical Tuning Reference](OpenDrift/docs/Tuning.md) for the setup order, symptom table, surface-profile workflow, and findings from real track testing. A simpler beginner guide is being prepared for the v1.0 release.

## Install Firmware

Use the [public OpenDrift browser installer](https://doublej380-pixel.github.io/OpenDrift/) to flash a supported board over USB without an account, source compilation, or command-line tools. Use Chrome or Edge on a desktop computer and disconnect the board from the receiver, servo, and external power before flashing.

## Current Features

- ESP32-S3 firmware using PlatformIO and Arduino.
- 280 x 456 AMOLED touch UI with a static RGB565 background and swipeable pages.
- Dedicated 250 Hz IMU/control/steering task isolated from UI, WiFi, and logging work.
- Continuous yaw-acceleration prediction with throttle-informed look-ahead.
- Quiet-drift reference feedback that yields to driver steering and throttle changes.
- Receiver steering input.
- Optional receiver throttle sensing with automatic Performance Mode behavior.
- Receiver gyro gain input.
- Selectable GPIO 18 gyro-gain input or throttle passthrough output.
- Servo output with center, reverse, and travel settings.
- Servo quiet band for direct-drive steering buzz.
- Steering calibration for max left, center, and max right.
- Separate radio steering travel limit.
- Separate servo reverse and gyro reverse.
- Gyro tuning:
  - gain
  - deadband
  - maximum correction
  - smoothing
  - Drift Memory and limit
  - Hold Assist
  - Prediction
  - Countersteer Assist
- Up to 12 persistent named surface/driving profiles.
- Scrollable trackside profile selection on both displays.
- Do-nothing signal-loss behavior when steering input is lost.
- WiFi access point.
- Browser-based web configurator.
- Temporary blackbox v10 CSV logging with OpenDrift v1.0 reference, prediction, throttle, and correction telemetry.
- Persistent settings stored in ESP32 preferences.

## Hardware Routing

### Supported Boards

The **Waveshare ESP32-S3 Touch AMOLED 1.64** is the final and primary OpenDrift hardware target. New development, UI work, and release testing target this board.

The older Waveshare 1.28-inch round display build is deprecated. Its PlatformIO environment and implementation remain in the repository for experimentation, but it may not receive new UI features or the same level of testing.

Current default pinout:

| Signal | GPIO | Direction | Notes |
| --- | ---: | --- | --- |
| Servo output | 16 | Output | Goes to steering servo signal |
| Receiver steering | 17 | Input | Standard RC PWM input |
| Receiver throttle | 15 | Input | Standard RC PWM input; always available to the blackbox |
| Gain input / throttle output | 18 | Selectable | Gain-channel input by default, or throttle passthrough output |

Make sure the receiver, ESP32 board, and servo power system share ground.

Throttle sensing is not required for basic stabilization, but it is strongly recommended. With the throttle signal connected, OpenDrift can release settled-drift features during power changes instead of inferring those events from yaw alone. Treat it like a sensored-motor cable: the fallback works without it, while Performance Mode has substantially better phase awareness.

The servo should be powered from a suitable BEC or ESC receiver rail. Do not rely on the ESP32 board to power a steering servo.

## Build And Upload

The firmware project is in:

`OpenDrift/`

Build with PlatformIO:

```sh
pio run
```

The default environment is `waveshare_amoled_164`. Build or upload it explicitly with:

```sh
pio run -e waveshare_amoled_164
pio run -e waveshare_amoled_164 -t upload
```

The custom PlatformIO board definition is included at `OpenDrift/boards/waveshare_amoled_164.json`.

The deprecated round-board environment remains available for experimentation:

```sh
pio run -e waveshare_128
```

The `waveshare_amoled_164_rescue` environment is a fallback recovery build using the generic ESP32-S3 DevKit definition. It is not the normal release target.

Main dependencies are managed in `OpenDrift/platformio.ini`:

- SensorLib
- LovyanGFX
- ESP32Servo
- CST816S (deprecated round-board build only)

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

### Core

Primary correction limits:

- `Deadband`: yaw-rate zone ignored around zero.
- `MAX CORRECTION`: maximum gyro correction in servo microseconds.
- `GYRO REV`: reverses only the gyro correction direction.

Use `GYRO REV` when normal steering direction is correct, but the gyro counter-steers the wrong way.

### Response

Fast controller behavior:

- `SMOOTHING`: the single time-based yaw low-pass.
- `PREDICTION`: continuous yaw-acceleration look-ahead.
- `SERVO QUIET`: suppresses very small physical servo-command changes.

### Assistance

Slow quiet-drift behavior:

- `COUNTERSTEER`: chooses how much additional steady countersteer OpenDrift carries. Zero preserves the driver-led base response; higher values reduce the steering load on the driver without increasing fast yaw damping.
- `HOLD ASSIST`: controls how firmly the quiet-drift reference is retained.
- `DRIFT MEMORY`: feedback for deviation from a learned quiet-drift reference.

The Drift Memory limit remains available in the web configurator as an advanced limit.

`SMOOTH` is intentionally inverted from raw filter math: higher numbers mean more smoothing and slower gyro response.

Deadband is applied as a soft deadband. Small yaw noise is still ignored, but correction fades in from zero instead of jumping as soon as yaw crosses the deadband value.

Conservative first-power values:

| Setting | Start |
| --- | ---: |
| Gain | 1.50 |
| Deadband | 4.0 |
| Max correction | 250 |
| Smoothing | 0.01 |
| Countersteer Assist | 0 |
| Drift memory | 0.00 |
| Memory limit | 80 |
| Hold Assist | 0 |
| Prediction | 0 |

Tune symptoms:

| Symptom | Try |
| --- | --- |
| Weaves left/right driving straight | Increase deadband or lower gain |
| Initial response is strong but runs out of authority | Raise Max Correction carefully |
| Fast response overshoots | Add Prediction in steps of 5 |
| Prediction makes transitions nervous | Lower Prediction |
| Car is stable but the driver carries too much countersteer | Raise Countersteer Assist in steps of 10 |
| Gyro feels too hands-on during a settled drift | Lower Countersteer Assist |
| Long drift slowly wanders | Add Hold Assist, then minimal Drift Memory |
| Transition carries the old drift | Lower Hold Assist or Drift Memory |
| Mid-drift wheel oscillation | Lower gain first; verify servo and chassis |
| Feels slow or lazy | Lower smoothing slightly or raise gain |

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

Basic firmware/system information. Tap the GPIO 18 mode button to switch between `GAIN INPUT` and `THROTTLE OUT`.

### Profiles

The Profiles page lists the driving profiles created in the web configurator. Tap a profile to activate its complete driving tune. Swipe vertically when more than four profiles exist; the list supports up to 12 profiles.

Profiles save gain, deadband, max correction, smoothing, Prediction, Countersteer Assist, Hold Assist, Drift Memory and its limit, and radio steering travel. Trackside adjustments automatically save back to the active profile.

Hardware and installation settings remain global, including gyro/servo direction, servo center and travel, receiver calibration, WiFi, logging, and GPIO mode. Switching surfaces therefore cannot disturb the car's physical setup.

## Web Configurator

When WiFi is enabled, OpenDrift starts a web configurator at:

`http://192.168.4.1/`

Current web settings:

- Create named driving profiles from the current tune
- Activate or delete existing profiles
- Gyro gain
- Deadband
- Reverse gyro correction
- Max correction
- Smoothing
- Prediction strength
- Drift memory
- Memory limit
- Hold Assist
- Servo reverse
- Servo center
- Servo travel
- Servo quiet band
- Steering max left / center / max right
- Radio steering travel
- Gain channel low / high
- GPIO 18 gain-input or throttle-output mode
- WiFi enabled on boot
- WiFi auto-off timeout
- Blackbox logging enabled
- Dedicated Slope / Terrain Assist A/B toggle; raw disturbance telemetry remains active in both modes

The web page also shows the active profile, live receiver pulse values for steering, throttle, and gain, plus the active GPIO 18 mode.

Use the web configurator when you want to make several changes quickly. Use the onboard UI when tuning trackside without a phone or laptop.

## Onboard Blackbox Log

OpenDrift can store CSV-style blackbox logs in onboard FFat flash storage at about 20 Hz while steering receiver signal is present.

Blackbox logging is disabled by default. Enable `Onboard logging` in the web configurator only when you want to collect data, then save settings.

To avoid disturbing gyro timing, log rows are buffered in RAM while driving. The control loop never auto-flushes the log to flash. Use the web configurator to flush, download, or clear the log after the run.

The primary AMOLED build uses a custom 16 MB partition layout with a 6 MB application slot and FFat storage for logging. After changing to this layout, do a full flash erase once before uploading if the board bootloops or the log storage acts strange.

The web configurator shows the current log size and provides:

- `Download CSV`
- `Flush Log`
- `Clear Log`

Log rows include:

- Time
- Raw yaw rate
- Filtered yaw rate
- Roll and pitch gyro rates (`gyro_x_dps`, `gyro_y_dps`)
- Raw X/Y/Z acceleration in g
- Total acceleration magnitude and high-frequency acceleration delta
- A filtered `surface_disturbance` score from `0.0` to `1.0`
- Raw gyro correction
- OpenDrift v1.0 correction
- Steering input and calibrated steering command
- Servo output
- Servo quiet band
- Throttle input
- Gain input and active gain
- Active deadband, max correction, smoothing, Prediction, Countersteer Assist, Hold Assist, Drift Memory, and memory limit
- Predicted yaw, quiet-drift reference, reference error, steady countersteer contribution, and memory correction
- Driver steering activity and throttle-prediction blend
- Controller phase (`0` idle, `1` entry, `2` settled, `3` transition) and reference-lock blend
- Steering/throttle/gain signal state and GPIO 18 mode

Suggested test workflow:

1. Connect to the `OpenDrift` WiFi network.
2. Open `http://192.168.4.1/`.
3. Enable onboard logging if it is off, then save settings.
4. Tap `Clear Log`.
5. Drive the car.
6. Reconnect to WiFi and tap `Flush Log`, or use `Download CSV` which flushes first.
7. Download `opendrift-blackbox.csv`.

The CSV can be pasted into a spreadsheet or plotted to see whether the car spun because of delayed correction, overcorrection, max correction saturation, noisy yaw, or steering/radio behavior.

`surface_disturbance` still combines sudden acceleration-vector changes, roll/pitch rate, and low-g unloading indicators for analysis. OpenDrift v1.0 logs it but does not currently apply terrain correction.

## Gyro Algorithm Overview

OpenDrift v1.0 runs this path at 250 Hz:

1. Read receiver steering, throttle, and IMU yaw.
2. Subtract calibrated gyro offset and apply soft deadband.
3. Apply one time-based yaw low-pass.
4. Estimate short-horizon yaw from filtered yaw acceleration.
5. Extend that horizon briefly when throttle predicts a chassis-load change.
6. Convert predicted yaw directly into correction with Gain.
7. Learn a slow yaw reference while driver steering and throttle are quiet.
8. Add optional Countersteer Assist from the slow learned reference only.
9. Apply Drift Memory only to error from that reference.
10. Prevent memory from pushing farther into correction saturation.
11. Clamp to Max Correction, optionally reverse, mix with calibrated steering,
    and output to the steering servo at 250 Hz.

Driver steering activity and throttle changes make the slow reference yield
immediately. Neither disables the fast direct damping path.

## Signal Loss Behavior

If steering receiver input is missing, OpenDrift does not send a new servo command.

This leaves the servo at the last commanded position. It is intentionally a simple do-nothing behavior while failsafe strategy is still being evaluated.

## Tuning

Use the [Technical Tuning Reference](OpenDrift/docs/Tuning.md) rather than tuning every setting at once. The most important lessons from development are:

- Establish a mechanically sound car before blaming the gyro.
- Set the servo's internal anti-wobble only as high as it can run without buzzing; high servo torque/power can amplify internal oscillation.
- Treat Gain as the primary surface-dependent setting and store each surface in a profile.
- Tune entries with Gain, Max Correction, and Smoothing before adding slow reference feedback.
- Add Prediction in small steps only after the direct response is understood.
- Use Hold Assist for sustained-drift reference retention, then add only the minimum Drift Memory required.
- Use Countersteer Assist to choose driver-led versus gyro-led settled drifts without retuning Gain.
- Connect throttle sensing so OpenDrift can anticipate power and load changes.

OpenDrift v1.0 is the first public controller. Start from the conservative
values in the tuning reference and save proven surface setups as profiles.

## Future Work

- **Servo resonance calibration:** add a stationary calibration mode that sweeps a small steering command across a safe frequency range and captures high-rate chassis IMU data. Because the servo is rigidly mounted to the chassis, its buzz and self-oscillation should be measurable without another sensor. The first version should compare vibration energy, dominant resonances, settling, and left/right behavior to help evaluate internal servo anti-wobble, torque, power, and response settings and recommend OpenDrift quiet-band and correction-rate settings. Command shaping or resonance compensation should only follow after repeatable measurements show that it can reduce vibration without adding harmful control-loop delay.

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
- `OpenDrift/docs/Tuning.md`: complete tuning and blackbox interpretation guide.
- `OpenDrift/assets/backgrounds`: flash-resident AMOLED UI background data.
- `OpenDrift/boards`: custom PlatformIO board definitions.

## License

See `LICENSE`.
