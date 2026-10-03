# IMU acquisition and wobble diagnostics

This experimental change targets acquisition and measurement, not a new tune.
It retains the existing gyro range, sensor ODR, selectable LPF modes, controller
math, servo driver, output pins, and normal blackbox format.

## Acquisition changes

- Enable the QMI8658 synchronous shadow-register locking mechanism, including
  SensorLib's required I2C AHB clock-gating command at initialization.
- Read sample availability, allow the documented 6 us lock delay at 896.8 Hz,
  then read the 24-bit sample counter, temperature bytes, accelerometer XYZ,
  and gyro XYZ in one burst ending at GZ_H, which releases the lock.
- Detect repeated counters and unavailable samples. Retain the previous sample
  on a no-data response; do not wait indefinitely for the next sensor update.
  Three consecutive stale/failed ticks invalidate yaw under the existing
  validity threshold, so a stalled sample stream cannot stay healthy forever.
- Attempt lock release following a failed burst. Count read errors separately
  from unavailable data and control-task bus-lock misses.
- Read back gyro range, ODR, LPF selection, sensor enables, and sync mode after
  configuration. Report a failed configuration rather than assuming it applied.

The protocol was checked against the QMI8658A datasheet's register locking
section as well as the supplied QMI8658C document. Hardware testing is still
required; host tests cannot verify the chip's actual lock timing.

Some hardware has failed SensorLib's locking setup despite successful normal
sensor configuration. This is not sufficient evidence of a dead sensor or an
unsupported feature: the exact handshake failure remains under investigation.
Locking failure now triggers a full sensor reset and explicit restoration of
the previous asynchronous SensorLib gyro/accelerometer reads. Register readback
still verifies range, ODR, LPF, both enables, and that sync is cleared. Failure
of that recovery still rejects initialization; checks are not simply bypassed.
The boot console reports the fallback and timing CSV shows `locked=0`. In this
mode gyro, accel, and counter are separate reads, so the counter does not prove
that they belong to one coherent sample. Acquisition/timing diagnostics and
paired SD dumps remain usable, but locked-read benefits are not active.

## Capture a test

1. Flash your usual hardware target with actuators disconnected. Confirm normal
   startup, steering direction, endpoints, and failsafe before driving.
2. Keep your current tune, LPF, servo frequency, and CH3 behavior unchanged for
   the first comparison. Enable the normal blackbox too.
3. In the web configurator, find **Control diagnostics** and press **Start
   diagnostic capture** immediately before the run. With an AMOLED CRSF build,
   the same capture can be armed from EdgeTX: select **Diagnostics**, press
   Enter, and confirm that its status changes to **RECORDING**.
4. Within 45 seconds, include a few seconds at rest, sustained left and right
   drifts, and several entries/transitions. Note roughly when wobble occurs.
5. Park. Capture stops after 45 seconds or when the buffer fills; you can also
   press **Stop capture**. Reload the page to see the current capture state.
6. With an SD card installed, press the screen's **Dump to SD card** button.
   The same dump action from the web configurator or the EdgeTX **Save Logs**
   row also saves both
   files. Wait for the dump to finish before restarting or entering USB mode.
   The card gets a matching numbered pair, for example:
   `opendrift-blackbox-0001.csv` and `opendrift-diagnostics-0001.csv`.
   Existing files are not overwritten. A dump stops any active timing capture
   and prevents restarting it until saving finishes. Without a timing capture,
   the normal blackbox is still saved on its own.
7. Alternatively, download **timing CSV** and the normal blackbox over WiFi.
   Do not power-cycle before saving/downloading the timing capture: it is
   initially RAM-only. Internal-flash dumps still save only the normal blackbox;
   the timing companion requires SD. Reconnecting WiFi does not erase RAM.

Starting another capture replaces the previous one. With sufficient PSRAM the
buffer holds 16,000 records (about 1 MB), sufficient for 45 seconds at 250/333 Hz.
Smaller PSRAM allocations are attempted if needed; the last-resort internal RAM
buffer holds 512 records, so captures can end much earlier on low-memory boards.
Download is refused while recording. CSV formatting and network writes are not
performed in the control task; the capture itself writes fixed-size RAM records.

Send both CSVs, board/build name, servo model, tune, output frequency, and whether
CH3 gain control was enabled. Mention any change in feel after this update.

## Reading the timing CSV

- `time_us`: control-iteration start time since boot, with 32-bit wraparound.
- `interval_us`: spacing between control iterations.
- `iteration_us`: elapsed work through the telemetry update, excluding the
  diagnostic record write and subsequent task sleep.
- `i2c_wait_us`, `imu_read_us`: bus-mutex wait and coherent-read duration.
- `sample_counter`, `sample_delta`: sensor's 24-bit sample counter and modular
  difference. This counter is **not a microsecond timestamp**. Multiple sensor
  ticks per controller tick are normal; a delta greater than one alone does not
  prove lost controller samples.
- `read_errors`, `no_data_count`, `i2c_misses`: cumulative counters since boot.
- `yaw_dps`: yaw actually supplied to the controller (including existing
  invalid-sample handling); `gain`: effective controller gain, including CH3.
- `correction_us`, `servo_us`: computed correction and commanded servo pulse.
  Neither measures the servo's physical motion or the actual PWM edge timing.
- `bus_acquired`, `read_ok`, `fresh`: separate bus-access, valid fresh-read,
  and new-counter indicators. During sensor settling, `read_ok` can be false
  even if `fresh` is true.
- `locked`: locking configuration was enabled/read back, not an independent
  per-sample measurement of the hardware lock-completion signal.
- `steering_signal`: steering input validity.
- `applied_lpf_mode`: verified hardware mode, 0 = 24 Hz, 1 = 120 Hz, 2 = off.

These fields can identify timing gaps, stale reads, gain chatter, and command
oscillation. They do not establish causality by themselves. Interrupt-driven
acquisition, FIFO, PCA behavior changes, and controller retuning are deliberately
not included in this first step.

## Host regression test

From the firmware directory:

```sh
g++ -std=c++11 -Itest/imu_host -Ilib/IMU test/imu_host/test.cpp lib/IMU/IMU.cpp -o /tmp/opendrift-imu-test
/tmp/opendrift-imu-test
g++ -std=c++11 -Itest/imu_host -Ilib/ControlDiagnostics test/imu_host/diagnostics.cpp lib/ControlDiagnostics/ControlDiagnostics.cpp -o /tmp/opendrift-diagnostics-test
/tmp/opendrift-diagnostics-test
```

This compiles the actual IMU implementation against a fake transport and tests
signed decoding/scaling, counter wrap, repeated/unavailable samples, short-read
lock cleanup, and LPF mode changes. The second test verifies capture freezing,
snapshot protection during SD export, CSV formatting, and timed capture stop.
No hardware is accessed; SD filesystem behavior needs an on-board test.
