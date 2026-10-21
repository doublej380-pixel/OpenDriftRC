# Developing OpenDrift parameters

User-facing parameter metadata lives in
`lib/ParameterCatalog/ParameterCatalog.cpp`. Its IDs and shared defaults live
in `ParameterCatalog.h`.

The catalog is the authority for:

- permanent CRSF parameter IDs;
- persisted preference keys;
- display names and units;
- default, minimum, maximum, precision, and increment values;
- parameter type, availability, and read-only status;
- Settings and GyroController input validation;
- CRSF metadata advertised to EdgeTX/GroundTX;
- numeric limits rendered by the web configurator.

`ParameterStore` is the live-value authority. It keeps one value indexed by
each permanent parameter ID, performs catalog clamping, tracks a generation
counter, and persists the complete set as one versioned Preferences blob.
`Settings` retains typed accessors for hardware semantics and compatibility,
but ordinary accessors are now thin views over the store.

## Adding a normal setting

1. Assign a new explicit value in `OpenDriftParameters::Id`. Never renumber or
   reuse a published ID. Leave a tombstone if a setting is retired.
2. Add its `Definition` to `ParameterCatalog.cpp`.
3. Add `PROFILE` to its flags if driving profiles should capture and restore
   it. Profile persistence, application, and validation are automatic.
4. Add a typed `Settings` accessor only when application code benefits from a
   readable name or the value requires hardware-specific translation.
   Ordinary CRSF reads/writes and Preferences persistence are automatic.
5. Add it to `Settings::ControllerSnapshot` when it affects the real-time
   control loop. The controller task copies the coherent snapshot only after
   the store generation changes; it must not read the generic store per tick.
6. Add it to the display/web layout where appropriate. Use
   `WebConfigurator::parameterInput()` for numeric web fields.
7. No EdgeTX edit is required for an ordinary value. The tool discovers
   available parameters, names, limits, precision, increments, and choices
   from CRSF. New IDs appear in Other Settings until placed in the Lua
   `groups` presentation table; this table controls section/order only, not
   values or limits. Only a new action or derived status needs special behavior.
8. Add blackbox columns only when the value or its intermediate output is
   useful for diagnosing vehicle behavior.

Actions and calculated status values still need explicit bindings because
they do not map to an ordinary stored value. Hardware rules such as protected
GPIO pins, AMOLED rotation mapping, endpoint reversal, and coupled gain limits
also remain explicit in `Settings` or `CrsfParameterDevice`.

## Storage and migration

The first boot after this refactor imports all existing per-key values without
changing the tune, then writes the versioned `paramStore` blob. Driving profile
v12 stores catalog-indexed values and automatically includes every definition
carrying `PROFILE`. Profiles from v1 through v11 are migrated on load.

Published IDs are array indices and over-the-air compatibility identifiers.
They must never be renumbered or reused, even after a parameter is retired.

The October 20 runtime pass also retires servo-rate ID 25 at fixed selection
index 1 (333 Hz). Its old 250 Hz value is sanitized on load. Throttle-rate ID 39
remains independent. Servo/control settings are cached in `ControllerSnapshot`;
physical calibration is checked separately so captures/resets update the output
even without a catalog-generation change. Logging-only sensor metrics are gated,
but acceleration-delta filtering for parked detection must always remain active.

Servo Quiet ID 11 is likewise reserved, hidden, and fixed at zero. Its legacy
CSV field is retained, but normal output code no longer suppresses small pulse
changes. Do not recycle the ID or reintroduce a quiet-band gate in the driver.

### Retired settings (2026-10-20)

Non-MPU6050 builds retain IDs 5/6/7/32 as `RETIRED` compatibility slots:
Drift Memory, Memory Limit, Hold Assist, and Gyro LPF. They are unavailable
to discovered menus, read-only, and fixed at 0/0/0/2 respectively. Keep their
min/max/default identical so profile application is also safe. Catalog clamps
sanitize imported storage and stale writes; controller memory/hold behavior is
compiled out. The QMI gyro starts with hardware LPF bypassed. Software smoothing
remains adjustable with a default of zero (existing tunes are not reset).

`OPENDRIFT_IMU_MPU6050` intentionally retains the original definitions, defaults,
filter choices, and controller behavior. Shared Lua groups still list these IDs
so MPU6050 parameters appear normally; other targets omit them through discovery.

## Validation and safety limits

Configuration values are validated by `ParameterStore` using the catalog.
Final servo endpoints, normalized output clamps, failsafe
neutral, sensor validity checks, and other physical safety limits remain
local to the hardware/control code. They are safety invariants rather than
user parameter ranges and must not be removed during catalog cleanup.

## Controller math

Extract functions around meaningful signal-processing stages, not around
every individual variable. The top-level update should show the controller
pipeline while stateful stages retain their history in `GyroController`.
Examples already separated include effective direct gain, steady drift
assist, and transition timing.
