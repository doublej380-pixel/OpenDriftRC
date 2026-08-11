# Tail Slide Speed

Tail Slide Speed is a secondary response adjustment. A value of `50` preserves
the Open Beta baseline behavior.

Tail Slide Speed is a centered rotation-speed trim. While the driver is
actively moving the steering, values below 50 increase fast yaw damping to slow
rotation and values above 50 reduce it so the chassis can rotate more freely.
It does not command rotation, reverse the gyro correction, or change the steady
Countersteer Assist setting.

The fast correction always retains at least 40% of its normal strength. The
effect is also reduced as OpenDrift recognizes a settled drift, concentrating
the experiment on entries and transitions instead of sustained cornering.

## First test

1. Keep the proven track profile unchanged and set Tail Slide Speed to `50`.
2. Confirm that the car still matches the Open Beta baseline.
3. Test `40` for slower rotation and `60` for faster rotation using the same
   entries and transitions.
4. Continue in steps of `10`; stop if the tail begins to overshoot or
   transitions become less predictable.
5. Capture a blackbox log at `50` and at the preferred experimental value.

Compare transition duration, peak yaw rate, overshoot after the direction
change, steering activity, and the new `tail_slide_blend` field. Avoid changing
Gain, Prediction, Hold Assist, or Countersteer Assist during the comparison.

Initial testing confirmed that the centered revision changes rotation in the
intended direction without disturbing the `50` baseline. Use matched track
logs to determine whether a non-center value improves repeatability rather
than only changing driver feel.

## Blackbox fields

- `tail_slide_speed`: saved setting from 0-100, centered at 50.
- `tail_slide_blend`: instantaneous signed -1 to 1 rotation-speed adjustment.
  Negative values add damping; positive values release it. It moves away from
  zero only while deliberate steering movement is detected.
