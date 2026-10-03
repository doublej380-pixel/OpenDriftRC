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
