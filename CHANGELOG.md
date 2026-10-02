# Changelog

## v1.0.9 - 2026-10-01

### October 1 acquisition, display, logging, and recovery updates

- Adds QMI8658 locked acquisition with availability checks, bounded lock delay,
  one counter/accel/gyro burst, repeated-sample detection, and failed-burst lock
  release. Verifies gyro range, ODR, LPF, sensor enables, and acquisition mode
  using hardware register readback instead of assuming configuration succeeded.
- Adds per-stage IMU startup diagnostics. Observed locking-handshake failures
  now trigger a full sensor reset and verified restoration of the previous
  asynchronous SensorLib reads instead of a boot loop. The underlying handshake
  failure remains under investigation; fallback samples explicitly report
  `locked=0` and are not claimed to be coherent locked samples.
- Invalidates yaw after three consecutive stale/failed sample ticks, rather
  than treating a non-advancing sample stream as healthy indefinitely.
- Adds an explicitly armed, up-to-45-second RAM diagnostic capture at every
  controller tick. Its CSV includes loop spacing/work time, I2C waits/read
  duration, sample counters/freshness, error counters, effective gain, yaw,
  correction, commanded servo pulse, and applied LPF. CSV formatting and
  downloads stay outside the real-time task; normal blackbox formats remain
  unchanged. Capture capacity falls back to smaller allocations if needed.
- Makes SD dumps save matching numbered blackbox and diagnostic CSV files.
  Screen, web, and radio dump actions share the same path. Active diagnostics
  freeze during export, capture restart is blocked until saving finishes, and
  progress spans both files. Existing files are preserved; internal-flash
  dumps remain blackbox-only. A timing capture must be armed before the run.
- Optimizes AMOLED swipes with reusable physical-layout PSRAM caches and
  sequential composition while preserving stationary backgrounds and panel
  transparency. Replaces fixed-frame release animations with elapsed-time
  easing and removes the additional post-transfer preview delay. Live Radio
  and Steering refresh changes from 250 ms to 50 ms. Serial swipe reports
  expose delivered FPS and mean composition/transfer times; 60 FPS is not
  guaranteed. The cache uses 766,080 bytes and has an uncached fallback.
- Enables USB maintenance on all four official AMOLED V1/V2 PWM/CRSF targets,
  not just the GPIO8 recovery target. Firmware updating works without an SD
  card; an inserted card adds the separate read-only log volume. Official
  builds now use native TinyUSB and dual 3 MB OTA slots while keeping NVS and
  FFat addresses unchanged. Existing official installs require a full
  PlatformIO upload to install the new partition table first.
- Reports two active MSC LUNs through TinyUSB's count-based callback, which
  TinyUSB converts to the correct zero-based maximum LUN value of one.
- Extends USB SD serial diagnostics with callback busy returns, request
  mismatches, invalid requests, callback wait time, and worker state so host
  stalls can be separated from successful low-level SD transactions.
- Serves the read-only USB SD volume one 512-byte sector per callback. TinyUSB
  continues larger READ(10) commands through short reads, while the shorter
  callback wait and Arduino SD single-block path reduce USB task starvation
  risk without changing normal SD mounting or blackbox logging.
- Reduces the AMOLED horizontal page-swipe commit distance from 40 to 24
  pixels while retaining horizontal direction validation.
- Adds momentum-aware AMOLED page swipes: preview begins after 6 pixels, short
  fast flicks commit using projected travel, and release velocity shortens the
  remaining page animation. Slow small movements still settle back.
- Reads the FT3168 point count and coordinates in one repeated-start I2C
  transaction and reduces the touch-release hold from 35 ms to 18 ms, lowering
  swipe sampling overhead and flick-release latency.
- Places the firmware updater on primary LUN 0 and the read-only SD card on
  LUN 1, and advertises the ESP32-S3 full-speed connection as USB 1.1 to avoid
  Windows 11 dual-LUN enumeration stalls while retaining both volumes.
- Disconnects maintenance-mode USB at the start of application setup and
  reconnects only after both LUNs are fully initialized, preventing Windows
  from caching the SD LUN's early unnamed/no-media state.
- Gates TinyUSB synchronously at its initial mount callback, before Arduino
  enters application setup, so Windows cannot probe either LUN during the
  framework's otherwise unavoidable pre-setup USB enumeration.
- Normalizes TinyUSB's command-relative READ(10) offset before accessing SD
  sectors, allowing Windows reads larger than the 4096-byte USB MSC buffer
  instead of rejecting their second and subsequent chunks.
- Batches aligned USB reads through the SD driver's multi-block command instead
  of issuing a separate SPI command for every 512-byte sector.
- Adds once-per-second USB serial telemetry for SD MSC read counts, failures,
  transfer sizes, callback latency, and failure stage while in maintenance mode.
- Raises the AMOLED SD SPI clock from 10 MHz to a conservative 20 MHz after
  diagnostics confirmed large host metadata scans were transfer-bound with no
  raw SD read failures.
- Defers USB SD telemetry until mass-storage reads have been idle for two
  seconds, preventing CDC diagnostics from competing with sustained MSC traffic
  on the shared full-speed USB controller.
- Moves USB SD reads out of TinyUSB's high-priority callback into a dedicated
  low-priority worker with a bounded handoff buffer. Slow card transactions can
  no longer freeze the USB task and maintenance UI together.
- Waits cooperatively for ordinary worker reads to complete as one MSC callback,
  avoiding rapid TinyUSB busy retries that Windows treated as an I/O error.
- Keeps explicitly requested USB maintenance mode latched across unexpected
  software/watchdog resets so actuator initialization cannot follow a failed
  maintenance session. Intentional restart and completed updates clear it.
- Periodically yields during sustained SD MSC scans so TinyUSB's high-priority
  task cannot starve ESP32 idle/watchdog work during long host metadata reads.
- Adds RTC-backed automatic maintenance recovery after three unfinished boots
  across software/watchdog resets. Normal startup is confirmed after 15 seconds
  in the main loop; intentional maintenance restarts do not count as failures.
  Recovery bypasses controller/actuator startup and SD mounting, and the USB
  updater can run without a functioning display. Cold power loss/brownout
  resets the audit; this is not recovery from an invalid app or bootloader.
- Adds acquisition, export-freezing, and boot-recovery host tests plus detailed
  IMU diagnostic and screen-performance testing documentation. Hardware tune,
  pinouts, controller math, and servo frequencies are not changed by this batch.


### Control response and driver authority

- Adds **Driver Priority**, which progressively yields only the fast direct
  gyro path as the driver moves farther from center. Countersteer Assist,
  Drift Memory, Max Correction, and the calibrated physical endpoints retain
  their authority. The default of `0` preserves the established response.
- Reworks **Transition Speed** into gain-independent correction-reversal
  timing. Lower values produce a calmer reversal, higher values produce a
  sharper reversal, and Gyro Gain no longer masks the adjustment as strongly.
- Smooths gyro correction as yaw settles back toward center, removing the hard
  final snap without slowing active drift correction or driver input.
- Prevents wheel-wobble zero-crossings from repeatedly arming transition mode.
  A reversal now requires the previous yaw direction to have remained definite
  for 0.25 seconds, allowing Anti Wobble to stay fully engaged instead of being
  held near its shallow transition guard depth.
- Adds selectable **1/10** and **Micro** Anti Wobble frequency ranges. The
  original 2.5-3.6 Hz mode remains the 1/10 default; Micro targets faster
  5-15 Hz steering oscillations found in 1/24-1/28 chassis.
- Removes the temporary gyro-output hysteresis experiment after the transition
  zero-crossing fix restored Anti Wobble at full output resolution.
- Changes fresh-install defaults for **Max Correction** and **Countersteer
  Assist** to `100`. Existing saved tunes and profiles are not overwritten.

### Precision outputs and failsafe safety

- Replaces integer-stepped steering output with a high-resolution fractional
  PWM path, eliminating visible command quantization while retaining 250 Hz
  and 333 Hz servo modes.
- Gives regenerated throttle output its own high-resolution LEDC driver and
  selectable 50 Hz, 250 Hz, or 333 Hz output rate. Steering and throttle use
  independent timers so changing one rate cannot corrupt the other signal.
- Separates requested steering position from the actual pulse driver state so
  endpoint capture and telemetry remain correct with fractional output.
- Hardens startup, signal-loss, and IMU-invalid behavior: servo and ESC outputs
  arm at neutral, CRSF freshness is checked before control is enabled, and
  invalid sensor samples cannot be forwarded into steering commands.
- Improves QMI8658 read-health tracking and LPF reconfiguration recovery,
  including settling periods and bounded retries after sensor faults.

### Calibration and settings reliability

- Makes steering endpoint capture atomic and immediately persistent, preventing
  partially updated calibration data or a delayed settings save from making
  captured endpoints appear to disappear after driving or power cycling.
- Adds validation and recovery for stored endpoint ordering and calibration
  state while preserving valid existing physical limits.
- Adds a web-configurator factory reset that clears the tune, profiles,
  endpoint calibration, GPIO mappings, and board settings deliberately.
- Makes settings and profile range validation use the shared parameter catalog
  at every subsystem boundary.

### Blackbox archive workflow

- Adds a parked-only **Save Log** command to CRSF/EdgeTX and the web
  configurator. Driving data remains in PSRAM while the car is moving and is
  copied to persistent storage only after the car is safely parked.
- Writes the archive incrementally with a temporary file, checksum, free-space
  check, backup/recovery path, progress reporting, and cancellation if the car
  moves before the save completes.
- Allows a saved binary archive to be downloaded later as CSV, making logs
  recoverable after WiFi trouble without introducing flash stalls into the
  control loop.
- Adds native microSD detection on the Waveshare AMOLED board. When a readable
  card is installed, parked dumps are written directly as CSV and internal
  flash remains the automatic fallback when no card is available.
- Keeps every SD dump as a separate sequential file such as
  `opendrift-blackbox-0001.csv`; existing sessions are never overwritten and
  numbering resumes after reboot.
- Adds a dedicated AMOLED **Blackbox** page with logging enable/disable, live
  RAM record/size/duration statistics, saved-log statistics, dump progress,
  storage target, and RAM-clear controls.
- Adds a matching web-configurator dump command and permits receiver-free bench
  capture in the private maintenance build for storage testing.

### WiFi and web configurator

- Reworks access-point client tracking to distinguish WiFi association from a
  completed DHCP lease, which prevents a half-connected client from being
  reported as ready.
- Adds bounded DHCP/AP recovery when a client remains stuck obtaining an IP,
  plus `opendrift.local` mDNS access and a configurable access-point name.
- Keeps WiFi timeout behavior tied to the most recent usable client and avoids
  disruptive recovery work while the control path is active.
- Adds current controls and help for Driver Priority, transition timing,
  Anti Wobble scale, throttle rate, display orientation,
  brightness, dim timeout, log archiving, and factory reset.

### AMOLED themes and backgrounds

- Adds a persistent AMOLED theme engine with light or dark text, seven accent
  palettes, and translucent panels designed for arbitrary backgrounds.
- Supports up to 16 persistent custom backgrounds in FFat while keeping the
  compiled galaxy image as an immutable recovery fallback.
- Adds background upload, selection, deletion, validation, and interrupted-file
  recovery to the onboard web configurator, plus a dedicated selection page on
  the AMOLED display.
- Supports browser-converted 456 x 280 RGB565 assets from the OpenDrift website;
  images are written only while the vehicle is stationary.
- Adds AMOLED 180-degree orientation, brightness, and idle-dim controls. Matrix
  builds support all four 90-degree status orientations.

### CRSF, EdgeTX, and GPIO

- Exposes Driver Priority, Micro/1/10 Anti Wobble mode, throttle output rate,
  display rotation, and parked log archiving through the
  current EdgeTX Lua tool.
- Preserves permanent published CRSF parameter IDs and serves names, limits,
  precision, steps, choices, and live values from the firmware catalog.
- Improves live parameter refresh and endpoint status synchronization between
  firmware, the AMOLED display, web configurator, and radio tool.
- Retains assignable GPIO1-GPIO8 CRSF auxiliary outputs while reserving pins
  used by a board target's steering, throttle, or CRSF UART functions.

### Hardware targets and interface prototypes

- Adds private PWM and CRSF ports for the headless Waveshare ESP32-S3-Matrix,
  including onboard 8x8 status animations, orientation control, Matrix-specific
  I2C/pin mappings, and a dedicated partition layout. These builds remain
  intentionally excluded from public release assets and the web flasher.
- Adds a private Waveshare AMOLED V2 CRSF recovery target that moves a damaged
  GPIO16 throttle output to GPIO8 without changing the official build pinouts.
- Gives all AMOLED targets an isolated USB maintenance mode backed by
  dual OTA app partitions. Its writable firmware volume accepts a complete
  `firmware.bin`, verifies it, selects the new partition, and reboots
  automatically; an installed microSD card is exposed separately as a genuine
  read-only log volume.
- Reports firmware-copy activity and errors on the AMOLED, disables manual
  restart while flash is being written, records the source OTA partition, and
  shows a one-time **Update Complete** confirmation only after the new
  partition actually boots.
- Makes the maintenance restart close both mass-storage devices and logically
  disconnect USB before rebooting. USB is re-enumerated only after its media
  are ready, reducing slow drive discovery on desktop Linux.
- Removes the private USB OTA test-payload target after update-path validation.
- Adds the experimental **GroundTX** surface-radio interface prototype for the
  RadioMaster MT12. It currently demonstrates safe, memory-only setup screens
  and mix workflows without modifying the active EdgeTX model.

### Developer architecture and diagnostics

- Introduces `ParameterCatalog` as the single authority for persistent keys,
  permanent CRSF IDs, names, units, defaults, ranges, precision, increments,
  choices, availability, and write permissions.
- Refactors Settings, GyroController validation, CRSF metadata, and web numeric
  inputs to consume the shared catalog instead of duplicating hard-coded
  constraints across the project.
- Adds a catalog-indexed `ParameterStore` as the single live and persisted
  value source, with versioned migration from every existing tune.
- Converts driving profiles to a catalog-driven v12 format. Parameters marked
  `PROFILE` are now captured, restored, and clamped automatically; profiles
  from v1 through v11 migrate in place.
- Makes ordinary CRSF reads and writes generic. Explicit handlers remain only
  for actions, derived status, coupled values, and hardware safety rules.
- Makes the EdgeTX tool discover the firmware's available parameters, names,
  ranges, steps, precision, and choices over CRSF instead of maintaining a
  second hard-coded list of ordinary settings.
- Adds an atomic parameter-generation snapshot for the real-time controller.
  The 250/333 Hz task applies a coherent tune only when values change instead
  of repeatedly reading and reapplying every setting on every control tick.
- Separates meaningful controller stages such as effective direct gain,
  steady-drift assist, and transition timing into named
  functions while keeping stateful signal history inside GyroController.
- Adds `docs/DEVELOPING_PARAMETERS.md` with the required workflow and CRSF-ID
  compatibility rules for contributors adding or retiring parameters.
- Expands blackbox diagnostics with Driver Priority setting/scale, effective
  direct gain, transition slew, sensor health, and the latest control states.

### Credits

- Theme-engine foundations and selected quality-of-life/reliability concepts
  were contributed by [J3vb](https://github.com/J3vb), including work reviewed
  from PR #4 and then integrated into OpenDrift's current architecture. J3vb
  also identified and contributed the transition/wobble zero-crossing fix in
  [PR #7](https://github.com/J3vb/OpenDriftRC/pull/7).
- Driver Priority, latency/control-loop analysis, and valuable saturation and
  response testing were contributed by
  [uarenotreal](https://github.com/uarenotreal).

### Release targets

- Public firmware remains focused on Waveshare AMOLED 1.64 V1 and V2, with PWM
  and full-duplex CRSF variants.
- Waveshare Round 1.28 remains deprecated, and the Matrix and personal recovery
  targets remain private development builds.

## v1.0.8 - 2026-09-03

### Lower-latency gyro experiments

- Adds selectable QMI8658 gyro-filter modes: the original `24 Hz`, a
  lower-latency `120 Hz` mode, and hardware LPF bypass for controlled testing.
- Makes Smoothing `0.00` a true software-filter bypass so hardware and software
  phase delay can be evaluated independently.
- Raises the supported gyro Gain ceiling from `3.00` to `6.00` while retaining
  the existing `0.50-3.00` Channel 3 mapping by default.

### Correction authority

- Refactors the controller to return a signed gyro correction instead of a
  centered pseudo-servo command, eliminating an unintended internal
  `1000-2000 us` saturation point.
- Redefines Max Correction across the full endpoint-to-endpoint steering span:
  `50%` can move from center to one endpoint, while `100%` can override one
  endpoint all the way to the other.
- Migrates existing Max Correction settings and saved profiles to preserve
  their real correction authority. An old displayed value of `74` becomes
  approximately `37` without weakening or doubling the tune.
- Stops Transition Speed from dynamically shrinking the Max Correction ceiling.
  It now shapes transition damping only.
- Keeps the combined driver-plus-gyro command and calibrated physical servo
  endpoints as the final hard safety limits.

### Channel 3 and EdgeTX

- Adds persistent `CH3 Gain Min` and `CH3 Gain Max` controls to the EdgeTX Lua
  tool and onboard web configurator, adjustable from `0.00` to `6.00`.
- Keeps Channel 3 authoritative while its receiver signal is valid and reports
  the resulting live gain consistently to the display and EdgeTX tool.
- Adds the gyro LPF selector and expanded `0.00-6.00` Active Gain range to the
  current `OpenDrift.lua` release asset.

### Blackbox and documentation

- Adds `gyro_lpf_mode` so matched filter tests identify the active sensor mode.
- Replaces ambiguous correction columns with `gyro_requested_us`,
  `gyro_limited_us`, and `gyro_applied_us`.
- Adds `correction_saturated` to distinguish Max Correction clipping from final
  steering-range saturation during entries and transitions.
- Updates the tuning reference and website for the new gain range, filter modes,
  full-span Max Correction behavior, tune migration, and test procedure.

### Release targets

- Publishes Waveshare AMOLED 1.64 V1 and V2 firmware with PWM and CRSF receiver
  support.
- The deprecated Waveshare Round 1.28 builds remain available at v1.0.7c and
  are not part of the v1.0.8 public release.

## v1.0.7c - 2026-08-30

### Physical steering limits

- Replaces receiver-range calibration with physical servo endpoint calibration.
- Captures the servo's actual left, center, and right PWM positions and uses
  them as the final asymmetric output map and hard safety clamp.
- Invalidates v1.0.7b receiver-range captures because they are not safe to
  reinterpret as physical servo limits.
- Redefines Max Correction as `0-100%` of calibrated physical steering travel;
  existing settings and profiles migrate from the previous +/-500 us scale.
- Makes Steering Travel affect driver input only, leaving gyro authority to
  Max Correction and the calibrated physical endpoints.
- Gives both AMOLED and round displays dedicated red/green physical endpoint
  pages and keeps the same captures synchronized with the EdgeTX tool.

## v1.0.7b - 2026-08-29

### Steering calibration

- Moves AMOLED steering calibration onto its own page with three large capture targets.
- Gives each endpoint a red-to-green confirmation state and reports missing signal or invalid endpoint ordering directly on screen.
- Fixes stale touch hitboxes that could interpret endpoint taps as swipe gestures.
- Normalizes captured left/right PWM values so reversed transmitter channels calibrate correctly.
- Persists shared left/center/right capture state and synchronizes calibration status between the AMOLED page, web configurator, CRSF device, and EdgeTX tool.
- Adds `Capture Left`, `Capture Center`, and `Capture Right` actions to `OpenDrift.lua` with an always-visible calibration status.

## v1.0.7 - 2026-08-27

### Control

- Replaces event-gated hunt damping with a narrow phase-aware dynamic notch targeting the measured 2.5-3.6 Hz wheel-wobble mode.
- Keeps a shallow guard active through entries and transitions, then blends smoothly to full settled-drift depth.
- Tracks notch center frequency only during suitable settled conditions so deliberate chassis motion does not retune the filter.
- Improves transition prediction and damping through the complete yaw reversal.
- Renames the user-facing **Hunt Strength** control to **Anti Wobble** and keeps its fresh-install default at `50`.
- Preserves existing saved tunes by retaining the compatible preference and profile storage layout.

### Reliability and interface

- Defers CRSF UART startup until the AMOLED, IMU, UI, and shared resources are ready.
- Adds a deliberate AMOLED hardware-reset sequence and startup settling delay for more reliable power-up and rapid power-cycle recovery.
- Makes WiFi auto-off count from the most recent client disconnect instead of initial startup.
- Enlarges the AMOLED steering endpoint buttons for easier trackside calibration.

### Blackbox and documentation

- Adds dynamic-notch residual, removed correction, envelope, latch, consistent-cycle, and tracked-frequency telemetry.
- Renames the saved blackbox control column from `hunt_strength` to `anti_wobble`.
- Updates the technical guide, web tuning guide, web configurator help, CRSF documentation, and EdgeTX tool for Anti Wobble and current Transition Speed behavior.

### Hardware

- Corrects the OpenDrift daughterboard regulator enable/feedback connections and updates the PCB routing to match the repaired schematic.
- Waveshare AMOLED 1.64 V1 and V2 PWM/CRSF builds are included.
- The deprecated Waveshare Round 1.28 firmware remains frozen at v1.0.2.

## v1.0.6 - 2026-08-17

- Added selectable 250/333 Hz control and steering-servo output.
- Improved transition authority and off-throttle prediction.
- Added the first bounded settled-drift hunt-suppression implementation and supporting blackbox telemetry.

## v1.0.5 - 2026-08-14

- Replaced internal-flash blackbox writes with non-blocking PSRAM circular logging.

## v1.0.4 - 2026-08-12

- Synchronized live channel-3 gain across the display, web configurator, CRSF telemetry, and EdgeTX tool.
