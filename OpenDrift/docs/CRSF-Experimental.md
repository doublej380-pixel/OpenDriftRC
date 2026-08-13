# CRSF Open Beta builds

These are separate firmware targets for direct CRSF control. Choose the CRSF
variant only when the receiver is wired through OpenDrift.

## Build target

- `waveshare_amoled_164_crsf`: full-duplex AMOLED target
- `waveshare_amoled_164_v2_crsf`: full-duplex AMOLED V2 target
- `waveshare_128_crsf`: full-duplex round-display target

CRSF settings are stored in the separate `OpenDriftCRSF` NVS namespace, so
they do not overwrite the corresponding PWM tune.

All targets use the complete path: bounded CRSF receive processing, parameter
telemetry, GPIO 16 ESC PWM, deterministic neutral behavior, and the
[EdgeTX tuning tool](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.2/OpenDrift.lua). The round target proved stable with a SpeedyBee SB Nano at
the MT12's F1000 packet rate after the receive loop was given an explicit byte
budget; the AMOLED target now uses the same CRSF implementation.

## SpeedyBee SB Nano wiring

Wire the receiver signals crossed, as required by a UART:

| SpeedyBee SB Nano | OpenDrift board | Purpose |
|---|---|---|
| TX | GPIO 17 | CRSF data into OpenDrift |
| RX | GPIO 18 | CRSF data/parameter telemetry from OpenDrift |
| GND | GND | Common signal ground |
| 5V | Vehicle BEC 5V | Receiver power |

That table applies to AMOLED V1 and Round. AMOLED V2 uses GPIO 1 for receiver
TX into OpenDrift and GPIO 2 for receiver RX/parameter telemetry. Do not use
GPIO 17/18 for external signals on V2 because Waveshare connects them to
IMU_INT2 and TP_INT.

OpenDrift outputs are:

| OpenDrift board | Connect to | Signal |
|---|---|---|
| GPIO 15 | Steering servo signal | 250 Hz PWM |
| GPIO 16 | ESC throttle signal | 50 Hz PWM |

Do not power the servo or ESC motor from the display board. A DIY display-board
installation needs a regulated 5 V supply; do not feed a 6 V or higher BEC
directly into the board. The daughter boards under development include their
own regulator. Use the vehicle's normal power wiring, make sure every device
shares ground, and verify the connector labels rather than relying on wire
colour or position.

## Initial channel map

- CRSF channel 1: steering
- CRSF channel 2: throttle
- CRSF channel 3: gyro gain

Channel values are decoded from native 11-bit CRSF frames. Existing steering
calibration, travel, gyro processing, web status, and blackbox logging consume
those decoded values through the same interfaces used by the PWM build.

## Failsafes

- A CRSF channel frame older than 50 ms is treated as signal loss.
- Steering centers when the link is lost.
- GPIO 16 emits no throttle PWM until a valid link has held throttle within
  50 microseconds of center for 500 ms.
- If the link is lost, the full build commands neutral throttle immediately.
  Reconnection requires another neutral hold before live throttle passes.

## [EdgeTX parameter tool](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.2/OpenDrift.lua)

Download [`OpenDrift.lua`](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.2/OpenDrift.lua) and copy it to `SCRIPTS/TOOLS/OpenDrift.lua` on the radio SD
card, then open **OpenDrift** from the [EdgeTX Tools menu](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.2/OpenDrift.lua). The current tool reads
and writes sixteen settings over full-duplex CRSF:

- saved gain, deadband, max correction, and smoothing;
- Drift Memory, memory limit, Hold Assist, and Countersteer Assist;
- Tail Slide Speed, Prediction, Servo Quiet, Steering Travel, Servo Travel,
  and Servo Center;
- Servo Reverse and Gyro Reverse.

Writes are acknowledged over CRSF, applied live, saved through the normal
delayed settings writer, and request an immediate redraw of the current gyro
screen.

## Problems found during validation

- An F1000 receiver can keep the UART continuously non-empty. CRSF receive work
  is bounded per update so it cannot starve the 250 Hz controller.
- Using the general servo allocator for simultaneous 250 Hz steering and 50 Hz
  throttle exhausted or cross-routed ESP32 timing resources. `EscOutput` now
  owns a dedicated 14-bit LEDC channel for throttle.
- CRSF settings use the `OpenDriftCRSF` NVS namespace so switching firmware
  variants cannot overwrite a PWM profile.

## First bench test

1. Remove the motor pinion or disconnect the motor from the ESC.
2. Flash the CRSF environment matching the connected display board with only
   that board connected by USB.
3. Wire and power the receiver, then bind it to the MT12.
4. Open the USB serial monitor at 115200 baud. The five-second CRSF report
   should show increasing frame counts, low CRC errors, link quality, and
   `throttle=LOCKED`.
5. Confirm steering, throttle, and gain move in the web configurator.
6. Hold throttle neutral for 500 ms and confirm the report changes to
   `throttle=ARMED`.
7. Turn the transmitter off. Steering and throttle must return to neutral and
   the report must return to `throttle=LOCKED`.
8. Open the [EdgeTX tool](https://github.com/doublej380-pixel/OpenDriftRC/releases/download/v1.0.2/OpenDrift.lua), change one harmless value, and confirm both the radio
   acknowledgement and gyro-screen refresh.

Only reconnect the motor after every applicable check passes.
