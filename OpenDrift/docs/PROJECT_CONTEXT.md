# OpenDrift project context for developers and AI assistants

series: **1.0.9**.

This is a curated handoff of the conversation history available to the current
assistant, repository documentation, and recent testing. It is **not** a
verbatim chat export or a guarantee that every past exchange is represented.
Historical user reports are distinguished from measured evidence. Read the
current source before acting: this snapshot will become stale.

## Start here

OpenDrift is open-source firmware for a tunable RC drift gyro. The aim is a
smooth, predictable car with configurable driver workload, not merely maximum
stabilization. Development has been driven by real driving, shared blackbox
logs, community testing, and iterative comparison with commercial gyros.

The owner reports excellent recent track performance, including leading long
trains and positive feedback from newer and experienced drivers. These are
valuable subjective validation, **not controlled proof of market superiority**.
The current priority is polish and reliability. Preserve the validated driving
baseline unless a requested change and evidence justify altering it.

Repository: <https://github.com/doublej380-pixel/OpenDriftRC>.
Public project website: <https://opendriftrc.com>.

The repository root contains README, CHANGELOG, and release/installer material.
The PlatformIO project is the nested **`OpenDrift/`** directory. Paths below are
relative to that firmware directory unless stated otherwise.

Read these alongside this handoff:

- [Tuning](Tuning.md): current user-facing control behavior and tuning workflow.
- [Hardware](Hardware.md): board revisions, pinouts, electrical precautions.
- [Parameter development](DEVELOPING_PARAMETERS.md): catalog/store architecture.
- [IMU diagnostics](IMU_DIAGNOSTICS.md): timing capture and interpretation.
- [Screen performance](SCREEN_PERFORMANCE.md): rendering changes and testing.
- [USB SD performance](USB_SD_PERFORMANCE.md): current mount-delay investigation.
- [Root changelog](../../CHANGELOG.md): versioned changes and contributor credits.
- [Root README](../../README.md): installation and maintenance workflows.

Old experimental documents may describe superseded behavior. Source and newer
changelog sections take precedence. Old chat pinouts and one-off firmware
instructions must not override the selected current build.

## Collaboration and safety expectations

- Review contributors' concepts and implement appropriate pieces within our
  architecture. The owner explicitly does **not** want wholesale fork merges.
- Preserve working controller behavior during UI, storage, and architecture
  refactors. Change one causal variable at a time when investigating wobble.
- Credit contributors; J3vb contributed reviewed QOL/bug-fix work and the
  important anti-wobble correction from PR #7. uarenotreal contributed the
  Driver Priority concept and useful sensor-latency/correction-authority
  investigations. Exact credits/references are in CHANGELOG.
- Never infer physical servo position from a commanded pulse or infer a
  hardware fault solely from a failed software initialization step.
- Before flashing or exercising outputs, park the car and disconnect actuator
  power. The owner previously suffered a spur-gear injury during powered setup.
- A previous AMOLED board became hot and stopped enumerating; the owner
  specifically identified the IMU as hot. The cause was never established.
  Do not assert firmware necessarily caused or could not cause that failure.
  A different apparent dead-board incident was resolved with factory firmware
  and a good USB cable. Distinguish these incidents.
- ESP32 GPIO is 3.3 V logic, not a 5 V-tolerant interface. Use the board's
  documented power input and a suitable regulated supply; servo power is
  external. Do not generalize a regulator IC's rating to the whole board.
- Preserve unrelated local edits and private artifacts. Commit/push, releases,
  and website deployment are separate actions requiring the owner's request.
  Building an experimental binary does not authorize publishing it.

## How we got here

### Early development and the simplified controller

The initial controller accumulated assist, filtering, hunt detection, and
terrain/load-transfer ideas during extensive trial-and-error driving. Some
early difficulties were mechanical: alignment, suspension geometry, grip,
surface temperature, and radio quality all affected the feedback plant.
An unnoticed alignment issue materially improved when corrected; this is why
mechanical sanity checks precede control changes, but must not be used to
dismiss reproducible firmware problems.

After track comparisons with a Yokomo V4 and driving another car with a Revox,
the owner requested a simpler rebuild: "Simplify, then add lightness." The
rewritten controller became the public v1; the old controller is considered
alpha, not a user-facing alternative that should be reintroduced.

Throttle sensing was originally intended as a temporary research aid. Testing
repeatedly showed better behavior with it connected, and the owner accepted
it as a useful supported feature. It remains optional electrically, with a
fallback when invalid/absent. Do not remove it because other drift gyros omit it.
It informs load-change prediction/assistance; it is not a closed-loop traction
controller using wheel-speed telemetry.

Countersteer Assist was added to let drivers choose how much steady-drift
steering work the gyro carries. The owner likes substantial driver control;
other drivers prefer more assistance. These are preferences, not defects.

### Published user-interface and authority fixes

- Profiles allow different surface tunes; web configuration creates them and
  the device can select them.
- Calibration was clarified as **physical servo endpoints**, not merely
  receiver-input range calibration. Capture Left/Center/Right status is shared
  between screen and radio; endpoint/reset/persistence bugs were investigated.
- Max Correction became authority over the **full calibrated steering span**.
  50% represents center-to-endpoint authority; 100% can override a driver
  command at one endpoint toward the other. Final physical clamps remain.
- High-yaw spinout claims led to reviewing early correction saturation.
  The proper solution retained physical safety limits rather than copying
  screenshots that commented out all clamps.
- Gain now supports 0–6. CH3's configurable minimum/maximum map its range;
  saved gain and live effective gain are distinct. Valid enabled receiver
  gain control overrides saved gain, and displays must show this accurately.
- Transition Speed was recentered around a neutral setting of 50, then
  reworked to affect reversal timing without losing usefulness at high gain.
- Sensor LPF choices are 24 Hz, 120 Hz, and Off. Software smoothing can be
  zero for a genuine bypass. More filtering is not automatically more stable:
  phase lag and scheduling matter.

### Wobble investigation and community work

Many sequential logs explored prediction, hunt activation, strength sweeps,
and a narrow dynamic notch. Wobble was sometimes reduced but not eliminated.
The public label changed from Hunt Strength to **Anti Wobble**, default 50.
1/10 and Micro modes accommodate different steering/chassis dynamics.
Micro-car alignment was also corrected after initial left/right asymmetry.

Some anti-wobble regressions followed 1.0.9 development. J3vb's PR #7 concept
was integrated and the owner confirmed it restored previous effectiveness.
The temporary gyro-output hysteresis experiment was then removed. Its CRSF
ID 41 stays retired; do not restore it as an unexplained extra damper.

Notchy output prompted custom precision steering and ESC output work. The
initial custom steering implementation had one-sided/startup movement faults
that were fixed before the owner approved it. The owner wants to retain the
custom driver. Do not revert it to mask a timing issue.

A subsequent strong lead was that oscillation remained even when another
developer reduced control math to gain times yaw. That motivated checking
acquisition, effective gain, and scheduling **outside** GyroController, instead
of piling more damping into the controller.

### October 3 breakthrough: shared touch/IMU I2C contention

Trackside capture established a scheduling problem on the tested AMOLED setup:
touch traffic could block IMU access and disrupt the 333 Hz loop. The fix:

- controller I2C mutex acquisition is nonblocking (previously up to 2 ms wait);
- touch polling is approximately 30 Hz idle / 125 Hz during active gestures;
- touch acquisition/probing is nonblocking;
- after 16 touch read failures, it goes offline and is reprobed every 5 seconds;
- recovery restores orientation; touch initialization precedes the IMU sequence.

The owner's trackside handoff reports before/after 45-second captures with
15,000 ticks each:

| Metric | Before | After |
| --- | ---: | ---: |
| IMU mutex misses | 3,397 (22.6%) | 0 |
| Successful fresh reads | 11,603 | 15,000 |
| p95 control interval | 4,481 us | 3,000 us |
| Maximum interval | 5,172 us | 3,039 us |
| Interval standard deviation | 737 us | 5.5 us |
| Iterations over 3 ms | 2,942 | 0 |

These numbers are from `TONIGHT_HANDOFF_2026-10-03.md` at the repository root,
not a fresh independent analysis of the raw CSVs in this handoff. The owner
reported dramatically smoother driving afterward. This identifies a major
observed timing fault, not proof that every possible future oscillation has
the same cause. **Do not undo this fix during unrelated polish.**

## Current architecture

`src/main.cpp` owns startup, build-specific pin routing, tasks, hardware setup,
and integration. Important libraries:

| Component | Responsibility |
| --- | --- |
| ParameterCatalog | Permanent IDs, names, defaults, limits, types, flags |
| ParameterStore | Indexed live values, clamp/dirty/generation/persistence |
| Settings | Typed accessors, profiles, hardware semantics, controller snapshot |
| GyroController | Stateful yaw correction, prediction, assists, notch, transition math |
| IMU / Touch | Shared I2C sensor acquisition and touch handling |
| Servo / EscOutput | Actuator pulse generation, limits, output behavior |
| CrsfInput / RadioInput | Receiver acquisition and validity |
| CrsfParameterDevice | CRSF metadata, settings, derived status and actions |
| UI / WebConfigurator | On-device and WiFi configuration views |
| BlackboxLogger / ControlDiagnostics | RAM driving records and high-rate timing records |
| BlackboxArchive / OnboardStorage | Parked exports, SD/filesystem and USB SD transport |
| UsbMaintenance / BootRecovery | Isolated USB updater/storage and failed-boot recovery |
| Backgrounds / MatrixStatus | AMOLED background assets / headless LED indications |

Normal parameter persistence/profile/CRSF work is catalog-driven. Keep the
controller's typed local snapshot: it is updated coherently when parameter
generation changes, not by locking/reading generic storage every control tick.
Special actions, derived status, endpoint semantics, and hardware transforms
still require explicit handling.

IDs are compatibility contracts. Never renumber/reuse them. ID 41 is a
tombstone; 42 is diagnostic capture and 43 is USB maintenance. Verify the
current catalog before allocating new IDs.

The radio script discovers values and metadata dynamically. Its latest
presentation-only `groups` table orders sections like the web configurator;
unknown future parameters remain available under Other Settings. Actions
retain explicit behavior. EdgeTX on the owner's radio does not expose global
`table`: use the tested insertion sort, not `table.sort`. Test with that
library disabled. GroundTX.lua is a separate surface-radio UI prototype,
not a second copy of the OpenDrift CRSF tuning tool.

## Hardware/build matrix

AMOLED V1 and V2 example/firmware images are not interchangeable. LCD CS is
GPIO9 on V1 and GPIO46 on V2. V2 peripheral interrupt wiring affects which
external pins are usable. Verify selected board/environment before flashing.

| Environment | Routing summary |
| --- | --- |
| waveshare_amoled_164 | V1 PWM: inputs 15/16, steering out 17, gain/throttle out 18 |
| waveshare_amoled_164_crsf | V1 CRSF: RX17 TX18, steering15 ESC16 |
| waveshare_amoled_164_v2 | V2 PWM: inputs 15/16, steering out 1, gain/throttle out 2 |
| waveshare_amoled_164_v2_crsf | V2 CRSF: RX1 TX2, steering15 ESC16 |
| waveshare_amoled_164_v2_crsf_oops_gpio8 | Private owner build: RX1 TX2, steering15, ESC8 |
| waveshare_s3_matrix_pwm | Private headless: inputs1/2, steering3, gain/throttle4 |
| waveshare_s3_matrix_crsf | Private headless: steering1 ESC2, RX3 TX4 |

The private GPIO8 workaround exists because the owner reports GPIO16 no longer
drives the ESC. Preserve it and protect GPIO8 from conflicting auxiliary use.
Do not export its pinout as the official V2 pinout.

Round Waveshare 1.28 targets were temporarily revived for the owner's testing
after an AMOLED failure, but are now removed from current source. Historical
release binaries are not authority for current targets. Matrix remains private
unless explicitly authorized for release.

CRSF channel mapping: 1 steering, 2 throttle, 3 gain. Link loss returns outputs
to neutral; throttle arming includes a neutral hold. Keep these safety paths.
250 Hz is the broader-compatibility default; 333 Hz is opt-in for supported
servos. ESC output has its own 50/250/333 Hz selection. Verify manufacturer
support, do not assume a digital servo accepts any rate.

## IMU facts and the failed locking experiment

Normal successful acquisition uses asynchronous SensorLib reads. Typical
verified readback: CTRL3 `0x63` (1024 dps range, 896.8 Hz ODR), CTRL7 `0x03`
(accel/gyro enabled, asynchronous); CTRL5 `0x11` for LPF mode0 and `0x71`
for mode1 on the tested setup.

An experimental synchronous/shadow-register locking path requires SensorLib's
CTRL9/AHB handshake. Real hardware repeatedly failed that handshake despite
successful normal gyro/accel configuration. The problem remains unresolved.
It is **not** proven to be a dead sensor or an unsupported chip feature.

Default `IMU::begin()` now selects the validated async path directly, avoiding
the failed experiment and reset on every boot. `begin(true)` retains the
explicit experiment and full reset/reconfiguration fallback. Configuration
verification and stale/error detection remain enabled.

In async mode, gyro/accel/counter reads are separate. Do not claim one coherent
locked frame or use the counter as proof of simultaneity. The 24-bit counter
is sensor ticks, not microseconds. At 896.8 Hz sensor / 333 Hz control, multiple
sensor ticks per controller iteration are expected.

## Logging, flash, SD, and USB

Internal flash writes previously caused noticeable system stalls. Formatting
smaller strings or moving work to the other core is not automatically a cure
for flash/cache effects. Current normal capture is RAM-first; save only while
parked. Do not stream flash writes during sustained slides.

Timing diagnostics are explicitly armed, up to 45 seconds at control rate,
normally 16,000 records in PSRAM. CSV formatting/export stays outside the
control loop. `servo_us` is the command, not measured servo motion or PWM edges.

With SD, Save Logs/Dump saves numbered blackbox and diagnostic companion CSVs,
preserving previous files. Capture freezes during export and cannot restart
mid-export. Without SD, internal-flash export saves the normal blackbox only;
download RAM diagnostics before power-cycling. Starting a new diagnostic
capture replaces the old RAM capture. Wait for SAVED/completion before restart.

All five AMOLED environments support USB maintenance without an SD card.
Matrix does not have this AMOLED maintenance implementation. In maintenance,
driving tasks/actuator outputs are inactive, firmware updater is LUN0, and SD
is read-only LUN1. The maintenance restart logically disconnects the drives;
a USB device cannot issue the host operating system's unmount command.

USB application updates require the correct application `firmware.bin`.
They cannot install a new partition table. Older single-app layouts require
one normal PlatformIO upload for the dual-app layout. Do not erase the chip or
invent flashing offsets. Full factory images and application-only images are
different artifacts. Preserve NVS/FFat/tunes and back up before migrations.

After three consecutive unfinished software/watchdog boots, automatic recovery
enters maintenance before IMU/output startup and skips SD mounting. A healthy
normal boot clears the audit after 15 seconds. Cold power/brownout starts a
fresh audit; manual mode changes do not count as failed boots. Recovery cannot
fix a corrupt bootloader/app that never executes.

## Current local polish and validation state

At creation of this document, local HEAD is `750ff2a`:
`Add trackside diagnostics and eliminate touch I2C jitter`. The polish below
was initially local/unpublished and is being committed with this handoff as
part of 1.0.9. This is a historical baseline, not a permanent claim about the
current HEAD or publication state. Recheck `git status` and `git log` before acting.

Recent local changes:

- Normal boot skips the failing IMU locking experiment; archived CSV startup
  statistics use 4 KB reads rather than one filesystem call per character.
  Boot timing is printed per stage. Owner confirms boot speed restored.
- Lua sections now match web groupings, with live gain/calibration retained.
  Initial `table.sort` crashed on EdgeTX; replaced with insertion sort.
  Owner confirms revised script works; desktop regression test disables `table`.
- USB SD callbacks now use up to 4 KB rather than forced 512-byte chunks.
  A completion semaphore replaces `delay(1)` polling. Physical SD reads remain
  single-sector CMD17 to avoid earlier CMD18 stalls. Adds throughput reporting.
- All five AMOLED builds compiled after USB changes. Compile/host-test success
  is not hardware proof of transport correctness or full delay resolution.
- Existing local boot/parser/IMU tests and Lua menu tests were run during their
  respective changes. Inspect documented commands and rerun after new edits.
- An unrelated local edit to the introductory comments of `platformio.ini`
  exists; preserve it. Private flasher artifacts and the trackside handoff are
  also present locally and must not be casually published.

User hardware currently uses the V2 private GPIO8 environment. Its local app:
`.pio/build/waveshare_amoled_164_v2_crsf_oops_gpio8/firmware.bin`.
An existing binary is not proof it matches subsequent source changes; rebuild
the selected target before supplying it.

## Deferred issue: slow SD folder opening over USB

The owner has explicitly decided this is **good enough for now**. Do not keep
working on it unless requested. Leave the working performance improvements in.

Observed evidence after those improvements:

- Reports of about 40 seconds between clicking SD and opening it; one supplied
  28-second video showed selection around 8 seconds and files around 25–26.
- Linux discovered both correct LUN identities/capacities and `sdb1` about one
  second after maintenance USB enumeration. An earlier blank/no-media instance
  belonged to normal-mode USB before the owner intentionally switched modes.
- Sample firmware report: 322 reads, 1,254,400 bytes, zero failures/busy returns/
  mismatches/invalid requests, max SD I/O 4,668 us, max callback wait 4,693 us,
  reported interval throughput 175 KiB/s. This is an excerpt, not a full
  correlated capture of the entire wait.
- Direct `udisksctl mount` tests took 5.79 and 4.84 seconds. A subsequent
  directory listing took 3.29 milliseconds; caching can affect this result.
- File browser still hesitated after mounting. Owner reports the issue on
  **both Windows and Linux**, so do not declare it a Linux-only browser fault.
- Kernel cache-page warnings fall back to write-through; they are not evidence
  of SD read errors and do not disable the host's read cache.
- UFW messages in the supplied kernel log were unrelated network traffic.

Root cause is **not established**. Remaining hypotheses include extra host
probing/file reads and USB/SCSI behavior; neither is confirmed. If resumed,
capture firmware activity across the full wait and correlate it with host
mount/open events. Do not format the card or alter SCSI descriptors blindly.

## Other open directions, not promises or implementation authority

- WiFi connection reliability remains affected on some setups when the ELRS
  transmitter is on. Turning it off helped the owner. RF interference is a
  plausible factor, not proof every WiFi failure has that cause. Do not turn
  this into automatic active radio-channel avoidance without a new design.
- SD is used for parked log dumps, not continuous driving-time streaming.
- Servo vibration calibration inspired by printer input shaping was discussed
  as a future idea, not a validated automatic servo-tuning feature.
- Automatic tuning, terrain compensation, squat/dive interpretation, onboard
  ELRS, ESC telemetry, custom serial servos, and Futaba SR/UR support were
  discussed, not all implemented. Inspect code before claiming support.
- Arbitrarily angled IMU mounting is not corrected merely by stationary bias
  calibration; do not promise arbitrary orientation support without a verified
  coordinate transform in current code.
- Custom AMOLED backgrounds can be uploaded through the web interface and
  stored separately from firmware. Storage writes belong in parked workflows.

## Suggested AI onboarding prompt

> Read OpenDrift/docs/PROJECT_CONTEXT.md, the root CHANGELOG and relevant linked
> docs completely. Inspect git status and the current source before proposing
> edits. Preserve the track-validated nonblocking touch/IMU timing fix and the
> precision output drivers. Keep published CRSF IDs, tune migration, endpoint
> clamps, failsafe and throttle arming intact. Distinguish tested facts from
> user reports and hypotheses. Do not resume the deferred SD-opening issue,
> publish private targets, merge forks wholesale, or release/deploy without my
> request. Tell me which build/files a proposed change affects and how you will
> verify it. Then help with my next specific task.

For another computer, share this repository state **including unpublished
changes** intentionally, plus any relevant CSVs separately. Machine-local
`/home/...` and old Windows paths in chat are not attached data on the next
machine. Raw chat exports, credentials, private flasher contents, and personal
test logs should be shared selectively, not automatically committed publicly.
