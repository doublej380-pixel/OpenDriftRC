# CH4 arming — development firmware

Current CRSF builds use receiver channel 4 as a dedicated drive-arm switch.
PWM builds are unchanged: adding this there requires an explicitly assigned
receiver input GPIO and an extra wire. No channel mapping or pinout is changed.

## Operation

- Configure a two-position switch: CH4 low (1000 us) is **PARKED**, high
  (2000 us) requests **ARMED**. Actual thresholds are <=1300 and >=1700 us;
  middle/ambiguous positions never permit driving. Intermediate pulses on an
  off -> on switch sweep preserve the previously observed off position. A middle
  position reached while armed disarms and requires low again.
- Boot with CH4 low. A valid low must be observed after boot, reconnect, or
  a sensor-health fault. Booting/reconnecting with CH4 high never arms.
- With valid link and healthy calibrated IMU, move CH4 high and hold throttle
  within 1500 +/-30 us for 500 ms. Drive permission also waits for ongoing
  archive saving to finish and for the main loop to suspend its UI/config work.
- CH4 low, link timeout, invalid yaw, repeated I2C misses, or revoked peripheral
  readiness immediately closes the control-task drive gate. Steering is held
  at its calibrated center and ESC output at 1500 us. Receiver values can still
  be inspected on the radio page; they cannot command these outputs while parked.
- Disarming wakes the display and resumes touch/settings/web services. Wake
  may take about 150 ms; output locking happens first, in the control task.
- Existing endpoint limits, throttle pulse rate, CRSF link timeout, and channel-3
  live gain remain in effect. Motion/filter history resets across arm transitions
  without replacing the measured stationary gyro bias.

## Display, touch, and configuration

While driving, AMOLED sleep is commanded, UI redraws/animations stop, and all
touch polling/recovery is suspended. A touch cannot wake the screen while armed.
The FT3168 remains electrically powered: deep sleep is deliberately not used
without a verified hardware wake/reset path on the shared sensor I2C bus.
See the [FT3168 datasheet](https://files.waveshare.com/wiki/common/DATA_SHEET_FT3168.pdf),
sections 2.2–2.3, for its multi-slave sleep/wake limitations.

Deferred settings saves and HTTP request handling are paused during the drive
session. Radio parameter reads remain available; writes/actions are rejected
and answered with the current value, not queued for surprise execution later.
CH3 gain still changes live during driving. Wi-Fi state is not changed by arming.
Blackbox capture continues; archive writes are never initiated while armed.

**Strict steering lock was selected by the owner.** Physical endpoint capture
and reset are disabled in the screen/radio interfaces for this mode; the servo
cannot sweep while parked, and a forced-center command must never be saved as
a left/right endpoint. Existing calibrated endpoints are preserved. New physical
endpoint calibration needs a separately authorized bench-mode workflow; do not
expect the parked capture buttons to move the servo or establish endpoints.

## Motor-disconnected bench validation (required before driving)

1. Set CH4 low. Boot; steering holds center and throttle holds neutral even when
   the wheel/trigger move. Verify screen/touch and parked configuration work.
2. Boot with CH4 high: outputs must stay locked and screen remain available.
3. Set CH4 low, pull throttle, then high: no drive. Release throttle to neutral;
   after the hold, the display sleeps and steering/ESC inputs become live.
4. Touch/swipe while armed: no UI activity. Check CH3 gain still works.
5. Set CH4 low while wheel/trigger are deflected: outputs immediately center/
   neutral, display wakes, and held touches do not trigger stale buttons.
6. Arm, turn transmitter off, then reconnect with CH4 still high: remain parked
   until low -> high and the neutral hold. Repeat several times.
7. Start an archive save while parked, then request arming: no drive during
   storage writes. Verify the save completes or safely cancels before driving.
8. Test both display rotations and a repeated arm/disarm cycle; check serial
   `Drive mode:` events and verify calibration data remains unchanged.

Serial `Arm status:` reports identify the blocking condition with decoded CH4
and throttle pulse widths and the UI/storage readiness flag. Radio -100/+100
usually decodes near 988/2012 us; neutral throttle should read 1470–1530 us.

Host tests exercise state/interlock logic and suspended-touch I2C behavior, not
physical PWM signals, panel current, actual radio failsafe, or electrical safety.
