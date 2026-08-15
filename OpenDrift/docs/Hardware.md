# OpenDrift Hardware Notes

## Primary board

The final supported target is the Waveshare ESP32-S3 Touch AMOLED 1.64. The older Waveshare 1.28-inch round display is deprecated and frozen; its existing source remains available for experimentation but receives no new releases or feature-parity work.

The AMOLED board has incompatible V1 and V2 revisions. V1 is marked at the top of the PCB and uses LCD_CS GPIO 9. V2 is marked beside the right-side headers and uses LCD_CS GPIO 46. V2 also connects IMU_INT2 to GPIO 17 and TP_INT to GPIO 18, so OpenDrift does not use GPIO 17/18 for external signals on V2.

## QMI8658 IMU

Both boards use the QMI8658 six-axis IMU. OpenDrift uses body Z as yaw and records body X/Y gyro plus all three accelerometer axes for terrain and load-transfer analysis.

| Board | SDA | SCL |
| --- | ---: | ---: |
| AMOLED 1.64 | GPIO 47 | GPIO 48 |
| Round 1.28 | GPIO 6 | GPIO 7 |

The current board orientation reports clockwise rotation as positive Z and counter-clockwise rotation as negative Z. Always verify correction direction by rotating the complete car before driving.

## PWM receiver and servo routing

AMOLED V1 and the Round build use the same PWM pinout:

| Signal | GPIO | Direction |
| --- | ---: | --- |
| Receiver steering / servo in | 15 | Input |
| Receiver throttle / throttle in | 16 | Input |
| Steering servo / servo out | 17 | Output at 250 Hz |
| Gain input or throttle passthrough | 18 | Selectable |

GPIO 18 can be a receiver gain input or an ESC throttle output, but not both.
To retain throttle sensing and receiver gain control simultaneously, split the
receiver throttle signal between GPIO 16 and the ESC instead of connecting the
ESC to GPIO 18.

AMOLED V2 PWM uses GPIO 15 steering input, GPIO 16 throttle input, GPIO 1 steering-servo output, and GPIO 2 as the selectable gain input or throttle output.

## CRSF routing

AMOLED V1 and Round CRSF share this routing:

| Signal | GPIO | Direction |
| --- | ---: | --- |
| CRSF RX from receiver TX | 17 | Input |
| CRSF TX to receiver RX | 18 | Output |
| Steering servo / servo port | 15 | Output at 250 Hz |
| ESC throttle / throttle port | 16 | Output at 50 Hz |

Both `waveshare_128_crsf` and `waveshare_amoled_164_crsf` enable the complete
full-duplex path. They remain separate from the normal PWM environments because
the GPIO routing and settings namespace differ.

AMOLED V2 CRSF uses GPIO 1 RX, GPIO 2 TX, GPIO 15 steering-servo output, and GPIO 16 ESC output. Its targets are `waveshare_amoled_164_v2` and `waveshare_amoled_164_v2_crsf`.

CRSF channel mapping is channel 1 steering, channel 2 throttle, and channel 3
gain. A stale channel frame centers steering and commands neutral throttle.
Throttle output requires a valid link and a 500 ms neutral hold
before arming.

## CRSF auxiliary channel outputs

The AMOLED CRSF builds can mirror any CRSF channel from 1 through 16 to a
standard 50 Hz receiver-style PWM signal. Assign each pin independently in the
WiFi web configurator under **Auxiliary Channel Outputs**.

| Board | Available auxiliary GPIOs |
| --- | --- |
| AMOLED V1 CRSF | GPIO 1–8 |
| AMOLED V2 CRSF | GPIO 3–8 |

GPIO 1/2 are unavailable on V2 because they carry the CRSF UART. Disabled pins
remain inputs. Enabled pins output the selected channel and command 1500
microseconds when the CRSF link is lost. GPIOs provide a 3.3 V signal only;
lights, controllers, or other accessories require their own appropriate power
supply and a common ground with OpenDrift.

## Throttle sensing

Throttle sensing is electrically optional and automatically falls back when no valid PWM signal exists. It is strongly recommended for OpenDrift v1.0 because it announces power and chassis-load changes, temporarily extends yaw prediction, and makes the slow drift reference yield before stale feedback can fight the transition.

Only the receiver signal and a shared ground are required. OpenDrift does not power the receiver or ESC through the throttle input.

## Power and grounding

The receiver, ESP32 board, and servo/ESC system must share ground. Power the steering servo from an appropriate BEC or receiver rail; do not draw servo current through the ESP32 board.

Feed a DIY OpenDrift display/development board with regulated 5 V on its 5 V
input. Do not connect a 6 V or higher BEC directly to that input. The OpenDrift
daughter boards under development include an onboard regulator so the builder
does not need to add a separate 5 V regulator. ESP32-S3 GPIOs remain 3.3 V logic
and are not 5 V tolerant; every external GPIO is a 3.3 V signal only.

Fast drift servos can draw large transient current and can oscillate from their own internal settings. Verify servo stability directly from the receiver before diagnosing the gyro.
