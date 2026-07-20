# OpenDrift

OpenDrift is an open source, phase-aware drift gyro for RC drift cars, built around the Waveshare ESP32-S3 Touch AMOLED 1.64 board. It reads steering, throttle, and optional gain channels from a receiver, mixes driver steering with gyro correction, outputs steering and optional throttle signals, and exposes tuning through both the onboard touch UI and a WiFi web configurator.

The goal is to make gyro setup less of a black box: OpenDrift distinguishes entry, settled drift, throttle transients, and transitions; exposes every meaningful behavior; and records enough telemetry to explain what the car did.

See the [OpenDrift Tuning Guide](OpenDrift/docs/Tuning.md) for the current setup order, symptom table, surface-profile workflow, and findings from real track testing.

## Current Features

- ESP32-S3 firmware using PlatformIO and Arduino.
- 280 x 456 AMOLED touch UI with a static RGB565 background and swipeable pages.
- Phase-aware IMU yaw-rate correction with idle, entry, settled, and transition states.
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
  - Hold Boost
  - Anti-Wobble
  - Hunt Damping
  - attack speed
  - return speed
- Up to 12 persistent named surface/driving profiles.
- Scrollable trackside profile selection on both displays.
- Do-nothing signal-loss behavior when steering input is lost.
- WiFi access point.
- Browser-based web configurator.
- Optional blackbox v5 logging with controller phase, throttle, six-axis terrain telemetry, and shadow surface-disturbance detection.
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

### Gyro

Basic gyro behavior settings:

- `Deadband`: yaw-rate zone ignored around zero.
- `GYRO REV`: reverses only the gyro correction direction.

Use `GYRO REV` when normal steering direction is correct, but the gyro counter-steers the wrong way.

### Gyro Tune

Trackside tuning page for correction strength and filtering:

- `MAX CORR`: maximum gyro correction in servo microseconds.
- `SMOOTH`: time-based low-pass filter amount. Higher values are smoother/slower.
- `DRIFT MEMORY`: slowly accumulated correction during sustained yaw. `0.00` is off.
- `MEMORY LIMIT`: maximum accumulated correction in servo microseconds.
- `HOLD`: extra high-yaw correction boost. `0` is off; higher values add more authority once the car is already rotating and settled.

`SMOOTH` is intentionally inverted from raw filter math: higher numbers mean more smoothing and slower gyro response.

Deadband is applied as a soft deadband. Small yaw noise is still ignored, but correction fades in from zero instead of jumping as soon as yaw crosses the deadband value.

### Response

Trackside tuning page for how quickly steering and gyro correction move:

- `ATTACK`: how quickly gyro correction can build, in fine 1-step adjustments.
- `RETURN`: how quickly gyro correction can return toward center, in fine 1-step adjustments.
- `DAMPER`: steering input damping in milliseconds. `0` is off; higher values calm faster driver steering changes.
- `WOBBLE`: near-center correction chatter suppression. It no longer smooths broad mid-drift correction changes; that job belongs to Hunt Damping.

### Stability

The Stability page contains `HUNT DAMPING`, which targets repeated mid-drift wheel oscillation without reducing entry authority. The controller must first enter its settled phase and observe repeated alternating fast-yaw excursions. A single entry, exit, or throttle change cannot activate it.

While hunting is detected, OpenDrift attenuates fast yaw peaks around the slow average rotation. The damped control yaw is never allowed to exceed the currently measured yaw magnitude, preventing old drift rotation from being carried through an exit. Idle, entry, throttle-transient, and transition phases bypass Hunt Damping.

`0` preserves the original controller behavior. Start around `40` or `50`, then raise it in steps of `10`. Very high values can make small mid-drift corrections feel muted, although the controller always retains part of the fast response.

Conservative first-power values:

| Setting | Start |
| --- | ---: |
| Gain | 1.50 |
| Deadband | 2.0 |
| Max correction | 250 |
| Smoothing | 0.10 |
| Drift memory | 0.00 |
| Memory limit | 120 |
| Hold | 0 |
| Attack | 80 |
| Return | 30 |
| Damper | 0 |
| Wobble | 50 |
| Hunt damping | 0 |

Tune symptoms:

| Symptom | Try |
| --- | --- |
| Weaves left/right driving straight | Increase deadband, lower gain, or raise smoothing |
| Spins out easily | Lower max correction or return speed first |
| Won't hold drift | Raise max correction slowly |
| Gradually loses authority during long drifts | Add a small amount of drift memory after gain and hold boost are tuned |
| Exits feel like correction hangs on too long | Lower drift memory or its memory limit |
| Transitions snap back too hard | Lower return speed |
| Steering input feels too abrupt | Increase damper |
| Repeated wheel oscillation after the drift is established | Raise Hunt Damping in steps of 10 |
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

Basic firmware/system information. Tap the GPIO 18 mode button to switch between `GAIN INPUT` and `THROTTLE OUT`.

### Profiles

The Profiles page lists the driving profiles created in the web configurator. Tap a profile to activate its complete driving tune. Swipe vertically when more than four profiles exist; the list supports up to 12 profiles.

Profiles contain gain, deadband, max correction, smoothing, Drift Memory and its limit, Hold Boost, Attack, Return, Anti-Wobble, Hunt Damping, steering damper, and radio steering travel. Trackside adjustments automatically save back to the active profile.

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
- Drift memory
- Memory limit
- Hold boost
- Attack speed
- Return speed
- Steering input damper
- Anti-wobble strength
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
- Slewed gyro correction
- Steering input and calibrated steering command
- Servo output
- Servo quiet band
- Throttle input
- Gain input and active gain
- Active deadband, max correction, smoothing, hold boost, anti-wobble, Hunt Damping, attack, and return
- Active drift-memory strength, limit, and correction (`i_gain`, `i_limit`, and `i_us` in the CSV)
- Hunt Damping control yaw, slow yaw, fast yaw, detection score, and engagement blend
- Controller phase (`0` idle, `1` entry/transient, `2` settled, `3` transition), settled blend, throttle-transient state, and active Hold Boost factor
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

`surface_disturbance` is currently shadow telemetry only. It combines sudden acceleration-vector changes, roll/pitch rate, and low-g unloading indicators. It does not alter gyro correction. Compare its peaks with spinouts, bumps, crests, dips, and camber changes before enabling any terrain compensation.

## Gyro Algorithm Overview

The current control loop works like this:

1. Read IMU yaw rate.
2. Subtract calibrated gyro offset.
3. Apply soft deadband.
4. Smooth yaw rate using a time-based tunable low-pass filter.
5. Classify the current behavior as idle, entry/throttle transient, settled drift, or transition.
6. Detect repeated alternating fast-yaw movement during settled drift and apply Hunt Damping only while that hunting is present.
7. Convert the resulting control yaw into correction using gyro gain. Hold Boost ramps in only during settled drift and is capped by current yaw.
8. Accumulate leaky, clamped Drift Memory only during settled drift; release it during entry, throttle transients, and transitions.
9. Limit correction with max correction.
10. Suppress only tiny near-center correction chatter.
11. Optionally reverse gyro correction.
12. Slew-limit correction using time-based attack and return speed.
13. Add gyro correction to calibrated steering command.
14. Apply servo center/reverse/travel and output to the servo.

This keeps driver steering as the base command and lets the gyro assist rather than fully take over.

## Signal Loss Behavior

If steering receiver input is missing, OpenDrift does not send a new servo command.

This leaves the servo at the last commanded position. It is intentionally a simple do-nothing behavior while failsafe strategy is still being evaluated.

## Tuning

Use the [OpenDrift Tuning Guide](OpenDrift/docs/Tuning.md) rather than tuning every setting at once. The most important lessons from development are:

- Establish a mechanically sound car before blaming the gyro.
- Set the servo's internal anti-wobble only as high as it can run without buzzing; high servo torque/power can amplify internal oscillation.
- Treat Gain as the primary surface-dependent setting and store each surface in a profile.
- Tune entries with Gain, Max Correction, Smoothing, and Attack/Return before adding settled-drift features.
- Use Hold Boost for sustained drift authority, then add only the minimum Drift Memory required.
- Use Hunt Damping for detected mid-drift oscillation; OpenDrift Anti-Wobble is only near-center chatter suppression.
- Connect throttle sensing for the best transition and power-change behavior.

A successful development reference on uneven asphalt used Gain `2.20`, Deadband `4`, Max Correction `400`, Smoothing `0.08`, Hold Boost `90`, Anti-Wobble `100`, Hunt Damping `50`, Attack `85`, Return `85`, and Drift Memory `0.00`. This is evidence that the controller works, not a universal tune; copy the process, not blindly the numbers.

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
