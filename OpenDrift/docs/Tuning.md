# OpenDrift Tuning Guide

This guide documents the current phase-aware controller and the practical lessons learned while developing it on asphalt and P-tile. A good tune starts with the car and servo, establishes clean base correction, and adds the settled-drift features one at a time.

OpenDrift can expose a bad chassis or servo setup, but it cannot tune around one. Make the car predictable first.

## Before Tuning

### Mechanical baseline

Confirm the following before changing gyro settings:

- Steering moves freely through its complete range.
- Tie rods, bellcranks, bearings, and servo saver have minimal play.
- The servo does not buzz or oscillate while connected directly to the receiver.
- Front alignment and caster are appropriate for the chassis.
- Rear suspension and tire setup provide a stable mechanical baseline.
- Receiver stability features such as TSM or AVC are disabled when possible.

Development testing showed that a poorly balanced chassis made both OpenDrift and a Yokomo V4 feel bad. Correcting the car's setup produced a larger improvement than any firmware setting.

### Servo configuration

Fast drift servos can create their own feedback loop. The Yokomo SP-02 used during development could violently oscillate when its internal anti-wobble, torque, and power settings interacted.

Set the servo's internal anti-wobble only as high as it can run without buzzing. Higher torque and power can amplify an unstable internal setting. Verify the servo directly on a plain receiver before diagnosing OpenDrift.

OpenDrift `CENTER QUIET` (formerly Anti-Wobble) is a different feature. It suppresses tiny near-center correction chatter and does not repair an unstable servo.

### Direction and calibration

With the wheels off the ground:

1. Calibrate steering left, center, and right.
2. Confirm normal transmitter steering direction.
3. Use Servo Reverse only if driver steering moves backward.
4. Rotate the car by hand and confirm the gyro counter-steers against that rotation.
5. Use Gyro Reverse only if gyro correction is backward.
6. Start with conservative steering and servo travel.

Do not drive until gyro correction direction is unquestionably correct.

## Throttle Performance Mode

OpenDrift operates without throttle sensing, but connecting the receiver throttle signal enables substantially better phase awareness. Treat the connection like a sensored-motor cable: sensorless fallback works, while the connected configuration is the intended performance setup.

Throttle does not directly command steering or represent vehicle speed. OpenDrift only uses sufficiently large pulse changes to identify short power transients. During those transients it releases settled-drift features instead of carrying the previous drift state into a lift, braking event, or power transition.

With a valid throttle signal:

- Hold Boost releases during power changes.
- Drift Memory stops accumulating and decays.
- Hunt Damping disengages.
- Base proportional gyro correction remains active.

If throttle signal is absent, the controller automatically falls back to yaw-based phase detection.

## Controller Phases

The controller separates driving into four phases:

| Phase | CSV value | Meaning | Active behavior |
| --- | ---: | --- | --- |
| Idle | `0` | Little meaningful yaw | Settled features reset or release |
| Entry/transient | `1` | New rotation or throttle change | Base correction; Hold, Memory, and Hunt release |
| Settled drift | `2` | Established rotation with stable power | Hold, Memory, and detected Hunt may operate |
| Transition | `3` | Rotation crosses direction | Previous drift state releases rapidly |

The phase system is why a high Hold value can now be usable without making every entry and transition feel sticky.

## What Each Setting Does

| Setting | Purpose | Primary symptom |
| --- | --- | --- |
| Gain | Proportional correction per degree/second of yaw | Overall gyro strength |
| Deadband | Softly ignores small yaw near zero | Straight-line twitch or weave |
| Max Correction | Caps gyro authority in servo microseconds | Excessive or insufficient maximum countersteer |
| Smoothing | Low-pass filters measured yaw; higher is slower | Noise versus response delay |
| Attack | Limits how quickly correction builds; higher is faster | Lazy or abrupt correction buildup |
| Return | Limits how quickly correction returns; higher is faster | Hanging or snapping correction |
| Hold Boost | Adds authority during established high-yaw drift | Long drift will not remain settled |
| Drift Memory | Slowly accumulates settled-drift correction | Gradual loss of authority in sustained drift |
| Memory Limit | Caps Drift Memory contribution in microseconds | Prevents accumulated correction from taking over |
| Hunt Damping | Attenuates detected alternating mid-drift yaw | Wheel hunting after drift is established |
| Center Quiet | Suppresses tiny correction chatter near center | Low-yaw servo-scale twitch |
| Input Damper | Smooths driver steering input in milliseconds | Abrupt transmitter input or transition stutter |

Drift Memory is internally logged using the historical `i_gain`, `i_limit`, and `i_us` CSV names. It is deliberately not a conventional all-purpose PID I term: it only accumulates during settled drift.

## Starting Values

Use these conservative values for a new installation:

| Setting | Initial value |
| --- | ---: |
| Gain | `1.50` |
| Deadband | `2` |
| Max Correction | `250` |
| Smoothing | `0.10` |
| Drift Memory | `0.00` |
| Memory Limit | `120` |
| Hold Boost | `0` |
| Attack | `80` |
| Return | `30` |
| Center Quiet | `50` |
| Hunt Damping | `0` |
| Input Damper | `0` |

These values prioritize a safe first test, not maximum performance.

### Proven development reference

The following tune drove extremely well on uneven asphalt with throttle sensing connected:

| Setting | Tested value |
| --- | ---: |
| Gain | `2.20` |
| Deadband | `4` |
| Max Correction | `400` |
| Smoothing | `0.08` |
| Drift Memory | `0.00` during the validated run |
| Memory Limit | `40` |
| Hold Boost | `90` |
| Attack | `85` |
| Return | `85` |
| Center Quiet | `100` |
| Hunt Damping | `50` |

This is a reference for expected controller behavior, not a universal setup. Chassis geometry, grip, servo speed, tire, and surface can require substantially different Gain.

## Recommended Tuning Order

### 1. Create a surface profile

Use the web configurator to create a named profile such as `Asphalt`, `P-tile`, or `Carpet`. Profiles capture the complete driving tune while leaving hardware calibration global.

Tap a profile on the onboard Profiles page to activate it. Swipe vertically when more than four profiles exist. Changes made on the touchscreen automatically save back to the active profile.

If the external gain channel is connected and selected, it can override the Gain stored in the active profile.

### 2. Establish base Gain

Set Drift Memory, Hold Boost, and Hunt Damping to zero. Begin with low Gain and perform gentle entries.

Increase Gain until the car catches rotation and holds countersteer without weaving or fighting driver input. Gain is the setting most likely to change between surfaces, so do not assume an asphalt value will suit P-tile.

If the car weaves near straight ahead, lower Gain or increase Deadband before touching advanced settings.

### 3. Set Max Correction

Max Correction controls available authority, not sensitivity near center.

Raise it if the gyro reaches its limit but the car still needs more countersteer. Lower it if the gyro can drive the wheels too far and create an overcorrection. Use the blackbox `gyro_raw_us` column to see whether the configured limit is actually reached.

Do not use Max Correction to repair straight-line weave.

### 4. Tune Smoothing

Lower values respond faster. Higher values filter more noise but add delay.

Raise Smoothing if the yaw signal is noisy or the car feels nervous. Lower it if entries and transitions feel late or lazy. Make changes of approximately `0.01–0.03` around a working tune.

### 5. Tune Attack and Return

Attack affects correction buildup. Return affects movement back toward center.

- Raise Attack if countersteer arrives too slowly.
- Lower Attack if correction strikes too abruptly.
- Raise Return if correction hangs after yaw falls.
- Lower Return if transitions snap back violently.

At high values such as `85/85`, development logs showed the slew limiter was nearly transparent. That is acceptable when the phase-aware controller is already clean.

### 6. Add Hold Boost

Use Hold Boost when entries work but long established drifts lose authority or will not settle.

Hold only builds in settled drift. Increase it in steps of `10`. If established drift becomes locked-in or reluctant to respond, lower it. A high value can be safe when phase detection is working, but confirm throttle sensing and transitions before assuming more is always better.

### 7. Add Drift Memory last

Start at `0.00`. Drive for at least 30–60 seconds and evaluate sustained drift, transitions, and exits.

If long drifts gradually lose authority after Gain and Hold are correct, try `0.01`, then increase in `0.01–0.05` steps. Use the smallest value that fixes the symptom.

Reduce Drift Memory or Memory Limit if:

- Correction hangs during exit.
- The next transition seems to remember the previous direction.
- Steering feels resistant after rotation falls.
- `i_us` frequently reaches the configured limit.

### 8. Add Hunt Damping only for real hunting

Hunt Damping targets repeated alternating yaw during settled drift. It does not activate from one entry, exit, or throttle change.

Start around `40–50` and adjust in steps of `10`. Reduce it if small mid-drift corrections feel muted.

The damped control yaw is safety-capped: it cannot exceed current measured yaw magnitude or carry the previous direction through zero. In the validated asphalt log, Hunt activated for only two short events totaling about one second instead of remaining engaged through most of the run.

Before raising Hunt, confirm the oscillation is not caused by the servo's internal anti-wobble or receiver stability system.

### 9. Use Center Quiet sparingly

Center Quiet operates only on tiny near-center correction changes. It is not the setting for sustained mid-drift hunting. Use Hunt Damping for that symptom.

### 10. Add Input Damper only if needed

Input Damper smooths transmitter input before gyro correction is mixed in. Add it lightly when driver steering changes upset the car. Too much can make transitions feel disconnected.

## Symptom Guide

| Symptom | Check first | Tuning direction |
| --- | --- | --- |
| Straight-line weave | Servo stability, receiver stability features | Lower Gain; raise Deadband or Smoothing |
| Countersteer arrives late | Smoothing and Attack | Lower Smoothing or raise Attack |
| Entry overcorrects | Direction, Gain, Max Correction | Lower Gain or Max Correction |
| High-speed entry spins | Blackbox correction timing and saturation | Determine whether gyro led or reacted to the spin before changing settings |
| Long drift loses angle | Correction saturation | Add Hold; then minimal Drift Memory |
| Mid-drift wheel hunting | Servo internal settings | Raise Hunt in steps of 10 |
| Transition stutters | Throttle sensing, Input Damper | Reduce Damper; check Hold/Memory release |
| Transition carries old steering | `i_us`, Hold factor, controller phase | Lower Memory or Hold; verify throttle input |
| Exit hangs | Drift Memory and Return | Lower Memory/limit or raise Return |
| Car reacts badly to throttle changes | Throttle signal state | Connect throttle Performance Mode and inspect transient state |
| Spinout follows bump, crest, or camber | Terrain telemetry | Compare acceleration/tilt event timing before altering control |

## Surface Profiles

Profiles are intended for the exact asphalt-versus-P-tile workflow that exposed the need for them.

Each profile stores:

- Gain and Deadband
- Max Correction and Smoothing
- Drift Memory and Memory Limit
- Hold Boost
- Attack and Return
- Center Quiet and Hunt Damping
- Input Damper
- Radio steering travel

Calibration, gyro/servo direction, servo center/travel, receiver endpoints, WiFi, blackbox state, and GPIO mode stay global.

For a new surface:

1. Activate the closest existing profile.
2. Create a new profile from that current tune in the web configurator.
3. Tune Gain first.
4. Recheck Max Correction and Smoothing.
5. Change Hold, Memory, or Hunt only when the matching symptom exists.

## Reading Blackbox v7

OpenDrift records at approximately 20 Hz while steering signal is present. Important columns include:

| Columns | Meaning |
| --- | --- |
| `yaw`, `filtered_yaw` | Raw and filtered yaw rate |
| `gyro_raw_us`, `gyro_slewed_us` | Requested correction before and after Attack/Return slew limiting |
| `steering_cmd_us`, `servo_us` | Driver command and final mixed output |
| `throttle_raw_us`, `throttle_transient` | Receiver throttle and detected power-change state |
| `control_phase`, `settled_blend` | Current phase and settled-feature engagement |
| `hold_factor` | Active Hold multiplier; `1.0` means no boost |
| `i_us` | Active Drift Memory correction |
| `hunt_score`, `hunt_blend` | Hunting detection confidence and damping engagement |
| `hunt_control_yaw` | Yaw actually used by proportional correction after Hunt processing |
| `gyro_x_dps`, `gyro_y_dps` | Roll and pitch rotation rates |
| `accel_x_g`, `accel_y_g`, `accel_z_g` | Raw acceleration axes |
| `accel_mag_g`, `accel_delta_g` | Total load and fast acceleration-vector change |
| `tilt_rate_dps` | Combined roll/pitch activity |
| `surface_disturbance` | Raw terrain/load-transfer score from `0.0–1.0`; remains available when assist is off |
| `terrain_active`, `terrain_assist` | Detected disturbance latch and blended controller release amount |
| `terrain_enabled` | `1` when slope/terrain assistance was enabled for this row; `0` for the A/B baseline |

### Diagnosing a spinout

Look at the order of events, not only the maximum values:

- If correction rises first and yaw accelerates afterward, investigate excess Gain, Hold, or Max Correction.
- If yaw accelerates first and correction follows, the gyro is reacting to a chassis or traction event.
- If correction reaches Max Correction for a sustained period, the controller is authority-limited; determine whether the requested direction was helpful before raising the limit.
- If an acceleration, tilt, or surface-disturbance spike precedes yaw, investigate a bump, crest, dip, camber transition, squat, dive, or tire unloading event.
- If `i_us` or Hold remains active into a transition, inspect throttle signal and phase selection.

### Slope / terrain assistance

The `surface_disturbance` score combines fast acceleration-vector changes, roll/pitch motion, and low-g unloading indicators. When assistance is enabled and the score crosses its trigger, OpenDrift temporarily releases settled-drift Hold, Memory, and Hunt behavior. Base gain and maximum correction authority are never reduced.

Use the dedicated **Slope / Terrain Assist** button in the web configurator for A/B testing. Turning assistance off immediately bypasses its controller effect, but raw disturbance telemetry continues to be logged. Check `terrain_enabled` before comparing runs.

Pitch correlated with throttle may eventually distinguish chassis squat and dive from road slope:

- Throttle increase plus a consistent pitch impulse suggests acceleration squat.
- Throttle lift or braking plus the opposite impulse suggests dive and rear unloading.
- Pitch movement without a corresponding throttle change suggests terrain or elevation change.

Pitch and roll telemetry should still be interpreted cautiously because road camber, chassis mounting angle, squat, dive, and bumps can produce similar signatures.

## Development Findings From Logs 20 and 21

The controller rewrite was driven by a specific failure in log 20: the previous Hunt filter could retain a large control yaw while measured yaw was already collapsing. That stale correction worsened recovery and could carry steering through zero.

The phase-aware controller tested in log 21 showed:

- No detected stale or unsafe Hunt control rows.
- Hunt intervention only during two short detected events.
- Substantially less correction saturation and servo endpoint clamping than the previous log, although the runs were not perfectly identical.
- Clean release of Hold and Memory during an observed direction transition.
- Materially better driving with throttle sensing connected.

The most important conclusion is architectural: base proportional correction should always remain available, while Hold, Memory, and Hunt must understand whether the car is entering, settled, changing power, or transitioning.

## Testing Discipline

- Change one feature at a time.
- Use a dedicated profile for each surface.
- Record the tune before and after each meaningful change.
- Test entry, sustained drift, throttle modulation, transition, and exit.
- Run each Memory or Hunt change long enough to encounter its intended condition.
- Use three quick throttle blips as a visible blackbox marker before a special test section.
- Report whether the gyro appeared to cause the event or react after the car had already lost grip.

Good tuning is repeatable. If a setting only helps one corner once, collect another run before treating it as the answer.
