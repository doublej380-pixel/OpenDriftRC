# Experimental ESP32-S3-Zero + MPU6050 port

Experimental development port, based on the headless Matrix targets. Not published
to GitHub releases or the webflasher. Compile/host-test success is not hardware
validation; start on USB with the motor, ESC, and servo disconnected.

## Supported board

Waveshare ESP32-S3-Zero / Zero-M **FH4R2: 4 MB flash, 2 MB QSPI PSRAM**.
The newer N8R8 Zero variant is not this target: its memory configuration needs
a separate build. The onboard single WS2812 is GPIO21. There is no display or
onboard IMU; connect an external MPU6050 breakout.

Sources:
- [Waveshare board documentation](https://docs.waveshare.com/ESP32-S3-Zero)
- [InvenSense MPU6000/6050 register map (manufacturer document mirrored by Open Impulse)](https://www.openimpulse.com/blog/wp-content/uploads/wpsc/downloadables/MPU-6050-register-mapping.pdf)

## Wiring

Shared sensor connection for both targets:

| MPU6050 breakout | S3-Zero |
| --- | --- |
| SDA | GPIO11 |
| SCL | GPIO12 |
| GND | GND, also shared with receiver/servo/ESC |
| VCC | Verified breakout supply; start with 3.3 V |
| AD0 | Low/GND for 0x68, or high/3.3 V for 0x69 |
| INT / XDA / XCL | Not connected |

The driver probes 0x68 then 0x69 and requires WHO_AM_I = 0x68. No external
interrupt connection is needed. Keep I2C leads short; it runs at 400 kHz.
MPU6050 breakout variants/regulators differ: verify the specific module's VCC
requirements and pull-up rail. **SDA/SCL and all ESP32 GPIO signals must remain
3.3 V logic**, even if a breakout accepts 5 V power. Do not connect a 5 V
pull-up or BEC to GPIO. Power the S3-Zero through USB for initial testing; use
regulated 5 V at the board's 5V pad for the bench/car setup, not at 3V3.
Power servos/ESC separately from the board's 3.3 V regulator.

Actuator/radio routing follows Matrix, not the AMOLED targets:

| Function | PWM target | CRSF target |
| --- | --- | --- |
| Steering receiver input | GPIO1 | CRSF channel 1 |
| Throttle receiver input | GPIO2 | CRSF channel 2 |
| Steering servo output | GPIO3 | GPIO1 |
| ESC output | GPIO4, if enabled | GPIO2 |
| Gain input | GPIO4, if ESC output disabled | CRSF channel 3 |
| Receiver TX → gyro RX | — | GPIO3 |
| Receiver RX ← gyro TX | — | GPIO4 |

PWM GPIO4 is gain input OR throttle output, not both. Split the receiver's
throttle signal to the ESC and GPIO2 if using GPIO4 for gain. Never join two
signal outputs. Do not confuse UART pins with the board's TX/RX labels,
which denote GPIO43/44 rather than this port's CRSF mapping.

## Build and flash

From the firmware directory:

```sh
pio run -e waveshare_s3_zero_mpu6050_pwm
pio run -e waveshare_s3_zero_mpu6050_crsf
pio run -e waveshare_s3_zero_mpu6050_crsf -t upload
```

Use a full PlatformIO upload for the first installation. Hold BOOT while
connecting USB (or BOOT + Reset) if necessary, then release BOOT. Serial
monitor is 115200 baud. These single-app 4 MB targets do **not** provide
AMOLED USB mass-storage maintenance or SD-card access. An app-only
`.pio/build/<environment>/firmware.bin` is not a full image for address 0x0.

Configuration: Wi-Fi `OpenDrift`, password `opendrift`, browser
`http://192.168.4.1`. CRSF also uses the current
`radio/edgetx/SCRIPTS/TOOLS/OpenDrift.lua`: it discovers the firmware's filter
choices automatically. Zero PWM/CRSF settings use separate NVS namespaces
from Matrix and AMOLED, so recalibrate physical steering endpoints and tune
this hardware rather than expecting another target's saved settings to load.

## Sensor behavior and limitations

- Shared controller, smooth return, precision outputs, prediction, and
  Anti Wobble math are unchanged. Raw Z gyro is yaw; mount the sensor flat
  with Z perpendicular to the floor. There is no arbitrary-angle compensation.
- Gyro ±1000 deg/s uses 32.8 LSB/(deg/s); accelerometer ±4 g uses 8192 LSB/g.
- One 14-byte accel/temperature/gyro register burst follows DATA_RDY at each
  control tick. No DMP or FIFO batching, and no equality-based rejection of
  legitimate identical samples while stationary. Partial reads never replace
  the last complete sample; three stale/failed ticks invalidate yaw.
- LPF indices remain 0/1/2, but labels on web/radio are **20 / 98 / 256 Hz**,
  not QMI's 24 / 120 / Off. 256 Hz is the MPU6050's widest documented mode,
  **not a true bypass**. Its 8 kHz gyro base is divided by 8; the other modes
  use the 1 kHz base undivided, giving 1 kHz output in all modes. The
  accelerometer shares the hardware filter configuration and updates at 1 kHz.
- Readback verifies identity, clock/power, ranges, filter, and divider.
  Failed filter configuration blocks acquisition until verified recovery.
- Diagnostic sampleCounter counts accepted bursts; it is not a hardware
  timestamp. counterDelta cannot measure missed sensor samples. locked stays
  false, since QMI locking is inapplicable here.
- RAM blackbox allocation adapts downward to available PSRAM. It will not have
  the AMOLED's 8 MB RAM capacity; logs must be downloaded before power-off.
- GPIO auxiliary outputs and SD/internal archived saves are not added by this
  port, matching the current Matrix feature boundary.

Single-LED status: cyan boot, amber calibration, green receiver ready, red no
signal, magenta initialization error. Matrix rotation controls are not exposed.

## Bench checklist before driving

1. Disconnect actuators; flash the matching target and open serial.
2. Confirm MPU6050 probe/configuration succeeds and PSRAM is detected. Keep
   the sensor stationary during bias calibration. No reset loop should occur.
3. Confirm web/radio shows 20/98/256 Hz choices; change each mode and verify
   the serial configuration message. Get a log for each after settling.
4. Rotate the sensor slowly in both yaw directions and confirm signed yaw and
   sensible magnitudes, with zero near rest. Check gyro correction direction.
5. With motor disconnected, attach a supported servo, start at 250 Hz, capture
   safe physical endpoints, and check smooth movement and input direction.
6. For CRSF, check throttle stays neutral until the neutral hold completes,
   then verify transmitter-off failsafe. Keep the motor disconnected.
7. First driving tests: modest gain, one LPF setting at a time, clean logs.
   Different sensor bandwidth/noise means a QMI tune may need adjustment.

Host regression test (no hardware substitute):

```sh
g++ -std=c++11 -DOPENDRIFT_IMU_MPU6050 -DOPENDRIFT_BOARD_ZERO \
  -Itest/imu_host -Ilib/IMU test/imu_host/mpu6050.cpp \
  lib/IMU/IMU.cpp lib/IMU/MPU6050.cpp -o /tmp/opendrift-mpu6050-test
/tmp/opendrift-mpu6050-test
```
