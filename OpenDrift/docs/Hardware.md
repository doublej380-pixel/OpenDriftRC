# OpenDrift Hardware Notes

## Primary board

The final supported target is the Waveshare ESP32-S3 Touch AMOLED 1.64. The older Waveshare 1.28-inch round display remains as a deprecated experimental build.

## QMI8658 IMU

Both boards use the QMI8658 six-axis IMU. OpenDrift uses body Z as yaw and records body X/Y gyro plus all three accelerometer axes for terrain and load-transfer analysis.

| Board | SDA | SCL |
| --- | ---: | ---: |
| AMOLED 1.64 | GPIO 47 | GPIO 48 |
| Round 1.28 | GPIO 6 | GPIO 7 |

The current board orientation reports clockwise rotation as positive Z and counter-clockwise rotation as negative Z. Always verify correction direction by rotating the complete car before driving.

## Receiver and servo routing

### AMOLED 1.64

| Signal | GPIO | Direction |
| --- | ---: | --- |
| Servo output | 16 | Output |
| Receiver steering | 17 | Input |
| Receiver throttle sensing | 15 | Input |
| Gain input or throttle passthrough | 18 | Selectable |

### Round 1.28

| Signal | GPIO | Direction |
| --- | ---: | --- |
| Servo output | 16 | Output |
| Receiver steering | 17 | Input |
| Receiver throttle sensing | 18 | Input |

The round board does not have a spare gain-channel input in the current routing. Its Gain comes from the saved setting/profile.

## Experimental CRSF routing

Both boards have isolated CRSF build targets. They share this logical routing:

| Signal | GPIO | Direction |
| --- | ---: | --- |
| CRSF RX from receiver TX | 17 | Input |
| CRSF TX to receiver RX | 18 | Output |
| Steering servo PWM | 16 | Output at 250 Hz |
| ESC throttle PWM | 15 | Output at 50 Hz |

The round `waveshare_128_crsf` target currently enables the complete full-duplex
path. The AMOLED `waveshare_amoled_164_crsf` target is deliberately compiled
RX-only/input-only until replacement AMOLED hardware is available for sustained
validation. Neither target replaces the normal PWM environments.

CRSF channel mapping is channel 1 steering, channel 2 throttle, and channel 3
gain. A stale channel frame centers steering and commands neutral throttle in
the full build. Throttle output requires a valid link and a 500 ms neutral hold
before arming.

## Throttle sensing

Throttle sensing is electrically optional and automatically falls back when no valid PWM signal exists. It is strongly recommended for OpenDrift v1.0 because it announces power and chassis-load changes, temporarily extends yaw prediction, and makes the slow drift reference yield before stale feedback can fight the transition.

Only the receiver signal and a shared ground are required. OpenDrift does not power the receiver or ESC through the throttle input.

## Power and grounding

The receiver, ESP32 board, and servo/ESC system must share ground. Power the steering servo from an appropriate BEC or receiver rail; do not draw servo current through the ESP32 board.

Fast drift servos can draw large transient current and can oscillate from their own internal settings. Verify servo stability directly from the receiver before diagnosing the gyro.
