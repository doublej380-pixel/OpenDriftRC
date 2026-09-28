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

## Adding a normal setting

1. Assign a new explicit value in `OpenDriftParameters::Id`. Never renumber or
   reuse a published ID. Leave a tombstone if a setting is retired.
2. Add its `Definition` to `ParameterCatalog.cpp`.
3. Add the stored value and typed getter/setter to `Settings`. Clamp at the
   setter with `OpenDriftParameters::clamp()`.
4. Bind the value in `CrsfParameterDevice::getScaledValue()` and
   `setScaledValue()`.
5. Apply it to its owning subsystem from the existing settings refresh path.
6. Add it to the display/web layout where appropriate. Use
   `WebConfigurator::parameterInput()` for numeric web fields.
7. Add its ID and any special presentation flags to the EdgeTX tool. The tool
   obtains the name, limits, precision, and increment from CRSF at runtime.
8. Add blackbox columns only when the value or its intermediate output is
   useful for diagnosing vehicle behavior.

Actions and calculated status values still need explicit bindings because
they do not map to an ordinary stored value.

## Validation and safety limits

Configuration values are validated at Settings/subsystem boundaries using
the catalog. Final servo endpoints, normalized output clamps, failsafe
neutral, sensor validity checks, and other physical safety limits remain
local to the hardware/control code. They are safety invariants rather than
user parameter ranges and must not be removed during catalog cleanup.

## Controller math

Extract functions around meaningful signal-processing stages, not around
every individual variable. The top-level update should show the controller
pipeline while stateful stages retain their history in `GyroController`.
Examples already separated include effective direct gain, steady drift
assist, and gyro-only output hysteresis.
