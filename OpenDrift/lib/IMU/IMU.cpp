#include "IMU.h"

#include <math.h>


bool IMU::begin()
{
    Wire.begin(SDA_PIN, SCL_PIN);


    if (!qmi.begin(
        Wire,
        QMI8658_L_SLAVE_ADDRESS,
        SDA_PIN,
        SCL_PIN))
    {
        Serial.println("QMI init failed: SensorLib detection/reset");
        return false;
    }

    auto stage = [](const char* name, bool ok) {
        Serial.printf("QMI init %s: %s\n", name, ok ? "OK" : "FAIL");
        return ok;
    };

    if(!stage("accelerometer config",
        qmi.configAccelerometer(
            SensorQMI8658::ACC_RANGE_4G,
            SensorQMI8658::ACC_ODR_1000Hz,
            SensorQMI8658::LPF_MODE_0
        ))) return false;

    if(!stage("gyroscope config",
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_1024DPS,
            SensorQMI8658::GYR_ODR_896_8Hz,
            SensorQMI8658::LPF_MODE_0
        ))) return false;

    gyroLpfMode = 0;
    lpfRetryMode = 0;
    lpfRetryCount = 0;
    lpfLastAttemptMs = 0;
    enableRetryMs = 0;
    settleUntilMs = 0;

    diagnostics = {};
    counterReady = false;
    if(!stage("disable accelerometer", qmi.disableAccelerometer())) return false;
    if(!stage("disable gyroscope", qmi.disableGyroscope())) return false;
    if(!stage("locking", configureLocking()))
    {
        // Locking is an acquisition enhancement, not a prerequisite for the
        // previously working gyro. Reset fully: a failed CTRL9 handshake can
        // leave a pending command or partial sync configuration behind.
        Serial.println("QMI warning: locking unavailable; restoring legacy asynchronous acquisition");
        diagnostics.locked = false;
        if(!stage("async fallback reset", qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, SDA_PIN, SCL_PIN))) return false;
        if(!stage("async accelerometer config", qmi.configAccelerometer(
            SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz,
            SensorQMI8658::LPF_MODE_0))) return false;
        if(!stage("async gyroscope config", qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_1024DPS, SensorQMI8658::GYR_ODR_896_8Hz,
            SensorQMI8658::LPF_MODE_0))) return false;
        if(!stage("async mode", qmi.disableSyncSampleMode())) return false;
    }
    if(!stage("enable accelerometer", qmi.enableAccelerometer())) return false;
    if(!stage("enable gyroscope", qmi.enableGyroscope())) return false;
    return stage("register verification", verifyGyroConfiguration(0));
}


bool IMU::readRegisters(uint8_t address, uint8_t* buffer, size_t length)
{
    Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
    Wire.write(address);
    if(Wire.endTransmission(false) != 0) return false;
    if(Wire.requestFrom((uint8_t)QMI8658_L_SLAVE_ADDRESS, length) != length)
    {
        while(Wire.available()) Wire.read();
        return false;
    }
    for(size_t i = 0; i < length; ++i) buffer[i] = Wire.read();
    return true;
}


bool IMU::configureLocking()
{
    // SensorLib performs the CTRL9 AHB-clock-gating handshake required for
    // I2C, as well as setting CTRL7.syncSmpl. Verify the resulting mode.
    if(!qmi.enableLockingMechanism())
    {
        Serial.println("QMI locking: sync/CTRL9 AHB handshake failed");
        return false;
    }
    uint8_t control = 0;
    diagnostics.locked = readRegisters(0x08, &control, 1) && (control & 0x80) != 0;
    Serial.printf("QMI locking CTRL7=0x%02X (sync bit must be set)\n", control);
    return diagnostics.locked;
}


bool IMU::verifyGyroConfiguration(uint8_t mode)
{
    uint8_t registers[3];
    if(!readRegisters(0x04, registers, sizeof(registers)))
    {
        Serial.println("QMI verification: CTRL3..CTRL5 burst read failed");
        return false;
    }
    uint8_t control = 0;
    if(!readRegisters(0x08, &control, 1))
    {
        Serial.println("QMI verification: CTRL7 read failed");
        return false;
    }
    uint8_t directLpf = 0;
    const bool directOk = readRegisters(0x06, &directLpf, 1);
    Serial.printf("QMI readback CTRL3=0x%02X CTRL5(burst)=0x%02X CTRL5(direct)=0x%02X direct_ok=%u CTRL7=0x%02X mode=%u\n",
        registers[0], registers[2], directLpf, directOk, control, mode);
    if((control & 0x83) != (diagnostics.locked ? 0x83 : 0x03)) return false;
    // CTRL3: +/-1024 dps, 896.8 Hz. CTRL5: optional gyro LPF enable/mode.
    if((registers[0] & 0x7F) != 0x63) return false;
    if(mode == 2) return !(registers[2] & 0x10);
    return (registers[2] & 0x70) == (mode == 1 ? 0x70 : 0x10);
}


bool IMU::readCoherentSample()
{
    const uint32_t started = micros();
    diagnostics.fresh = false;
    diagnostics.counterDelta = 0;
    if(!diagnostics.locked)
    {
        // Explicit fallback to the original SensorLib acquisition path. These
        // two reads are NOT asserted to belong to one locked sensor sample.
        uint8_t counterBytes[3];
        const bool gyroOk = qmi.getGyroscope(gyroX, gyroY, gyroZ);
        const bool accelOk = qmi.getAccelerometer(accelX, accelY, accelZ);
        const bool counterOk = readRegisters(0x30, counterBytes, sizeof(counterBytes));
        const bool ok = gyroOk && accelOk && counterOk;
        if(ok)
        {
            const uint32_t counter = counterBytes[0] | ((uint32_t)counterBytes[1] << 8) |
                ((uint32_t)counterBytes[2] << 16);
            diagnostics.counterDelta = counterReady ? (counter - diagnostics.sampleCounter) & 0xFFFFFF : 0;
            diagnostics.fresh = !counterReady || diagnostics.counterDelta != 0;
            diagnostics.sampleCounter = counter;
            counterReady = true;
        }
        else ++diagnostics.readErrors;
        diagnostics.readUs = micros() - started;
        return ok;
    }
    uint8_t status = 0;
    bool ok = readRegisters(0x2D, &status, 1);
    if(ok && !(status & 0x01))
    {
        ++diagnostics.noDataCount;
        diagnostics.readUs = micros() - started;
        // No fresh data is different from a bus failure. Retain the last
        // sample, but make its freshness explicit in the diagnostic capture.
        return true;
    }
    if(ok)
    {
        // At 896.8 Hz the documented lock delay is 6 us. Do not wait for
        // another ODR tick or busy-poll indefinitely inside the control task.
        if(!(status & 0x02)) delayMicroseconds(6);
        uint8_t bytes[17];
        // Timestamp + temperature + accel XYZ + gyro XYZ, ending at GZ_H.
        // Reading GZ_H releases the sensor shadow-register lock.
        ok = readRegisters(0x30, bytes, sizeof(bytes));
        if(ok)
        {
            const uint32_t counter = bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16);
            const uint32_t delta = counterReady ? (counter - diagnostics.sampleCounter) & 0xFFFFFF : 0;
            diagnostics.fresh = !counterReady || delta != 0;
            diagnostics.counterDelta = delta;
            diagnostics.sampleCounter = counter;
            counterReady = true;
            auto raw = [&bytes](int offset) -> int16_t {
                return (int16_t)((uint16_t)bytes[offset] | ((uint16_t)bytes[offset + 1] << 8));
            };
            const float accelScale = qmi.getAccelerometerScales();
            const float gyroScale = qmi.getGyroscopeScales();
            accelX = raw(5) * accelScale;
            accelY = raw(7) * accelScale;
            accelZ = raw(9) * accelScale;
            gyroX = raw(11) * gyroScale;
            gyroY = raw(13) * gyroScale;
            gyroZ = raw(15) * gyroScale;
        }
    }
    if(!ok)
    {
        ++diagnostics.readErrors;
        // A failed burst must not strand the device with its sample locked.
        uint8_t ignored;
        readRegisters(0x40, &ignored, 1);
    }
    diagnostics.readUs = micros() - started;
    return ok;
}


bool IMU::setGyroLpfMode(uint8_t mode)
{
    mode = constrain(mode, 0, 2);

    if(
        !qmi.isEnableGyroscope() &&
        millis() - enableRetryMs >= 1000
    )
    {
        enableRetryMs = millis();

        if(qmi.enableGyroscope())
        {
            settleUntilMs = millis() + GYRO_SETTLE_MS;
        }
    }

    if(mode == gyroLpfMode)
    {
        return true;
    }

    if(mode != lpfRetryMode)
    {
        lpfRetryMode = mode;
        lpfRetryCount = 0;
        lpfLastAttemptMs = 0;
    }

    if(lpfRetryCount >= 5)
    {
        return false;
    }

    if(
        lpfRetryCount > 0 &&
        millis() - lpfLastAttemptMs < 1000
    )
    {
        return false;
    }

    SensorQMI8658::LpfMode sensorMode =
        mode == 1
        ? SensorQMI8658::LPF_MODE_3
        : (mode == 2
            ? SensorQMI8658::LPF_OFF
            : SensorQMI8658::LPF_MODE_0);

    bool configured =
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_1024DPS,
            SensorQMI8658::GYR_ODR_896_8Hz,
            sensorMode
        );

    if(!qmi.isEnableGyroscope())
    {
        configured = qmi.enableGyroscope() && configured;
    }

    configured = verifyGyroConfiguration(mode) && configured;

    if(!configured)
    {
        lpfLastAttemptMs = millis();

        if(lpfRetryCount < 255)
        {
            lpfRetryCount++;
        }

        return false;
    }

    gyroLpfMode = mode;
    lpfRetryCount = 0;
    lpfLastAttemptMs = 0;
    settleUntilMs = millis() + GYRO_SETTLE_MS;

    return true;
}


uint8_t IMU::getGyroLpfMode() const
{
    return gyroLpfMode;
}



void IMU::update()
{
    uint32_t now = micros();

    float dt = 0.01f;

    if(lastUpdateMicros != 0)
    {
        dt =
            (now - lastUpdateMicros)
            /
            1000000.0f;

        dt = constrain(
            dt,
            0.001f,
            0.05f
        );
    }

    lastUpdateMicros = now;

    if(!readCoherentSample())
    {
        if(gyroReadFailures < 255)
        {
            gyroReadFailures++;
        }
        if(accelReadFailures < 255) accelReadFailures++;

        gyroReadOk = false;
        return;
    }

    gyroReadOk = diagnostics.fresh;
    if(!diagnostics.fresh)
    {
        // Reusing one old sample indefinitely would leave apparently healthy
        // feedback if the sensor stopped advancing. Preserve the existing
        // three-tick yaw-validity limit without counting this as an I2C error.
        if(gyroReadFailures < 255) gyroReadFailures++;
        if(accelReadFailures < 255) accelReadFailures++;
        return;
    }
    gyroReadFailures = 0;
    accelReadFailures = 0;

    accelMagnitude = sqrtf(
        (accelX * accelX) +
        (accelY * accelY) +
        (accelZ * accelZ)
    );

    tiltRate = sqrtf(
        (gyroX * gyroX) +
        (gyroY * gyroY)
    );

    if(!accelFilterReady)
    {
        slowAccelX = accelX;
        slowAccelY = accelY;
        slowAccelZ = accelZ;
        accelFilterReady = true;
    }

    float slowAmount =
        1.0f - expf(-dt / 0.25f);

    slowAccelX +=
        (accelX - slowAccelX) * slowAmount;
    slowAccelY +=
        (accelY - slowAccelY) * slowAmount;
    slowAccelZ +=
        (accelZ - slowAccelZ) * slowAmount;

    float deltaX = accelX - slowAccelX;
    float deltaY = accelY - slowAccelY;
    float deltaZ = accelZ - slowAccelZ;

    accelDelta = sqrtf(
        (deltaX * deltaX) +
        (deltaY * deltaY) +
        (deltaZ * deltaZ)
    );

    // Terrain detector used to release settled-drift features during a hard
    // compression, unload, or pitch/roll impulse. It never creates steering
    // correction directly.
    float accelerationScore = constrain(
        (accelDelta - 0.06f) / 0.50f,
        0.0f,
        1.0f
    );

    float tiltScore = constrain(
        (tiltRate - 15.0f) / 180.0f,
        0.0f,
        1.0f
    );

    float unloadScore = constrain(
        (0.75f - accelMagnitude) / 0.55f,
        0.0f,
        1.0f
    );

    float scoreTarget = max(
        accelerationScore,
        max(
            tiltScore * 0.80f,
            unloadScore
        )
    );

    float scoreTimeConstant =
        scoreTarget > surfaceDisturbanceScore
        ?
        0.04f
        :
        0.20f;

    float scoreAmount =
        1.0f - expf(-dt / scoreTimeConstant);

    surfaceDisturbanceScore +=
        (scoreTarget - surfaceDisturbanceScore)
        *
        scoreAmount;

    surfaceDisturbanceScore = constrain(
        surfaceDisturbanceScore,
        0.0f,
        1.0f
    );
}


bool IMU::isSettling() const
{
    return
        settleUntilMs != 0 &&
        (int32_t)(millis() - settleUntilMs) < 0;
}


bool IMU::isYawValid() const
{
    return gyroReadFailures < 3 && !isSettling();
}


bool IMU::lastGyroReadOk() const
{
    return gyroReadOk && !isSettling();
}


bool IMU::isHealthy() const
{
    return gyroReadFailures < 25;
}


bool IMU::isAccelHealthy() const
{
    return accelReadFailures < 25;
}



float IMU::getGyroX()
{
    return gyroX;
}



float IMU::getGyroY()
{
    return gyroY;
}



float IMU::getYawRate()
{
    return gyroZ;
}


float IMU::getAccelX()
{
    return accelX;
}


float IMU::getAccelY()
{
    return accelY;
}


float IMU::getAccelZ()
{
    return accelZ;
}


float IMU::getAccelMagnitude()
{
    return accelMagnitude;
}


float IMU::getAccelDelta()
{
    return accelDelta;
}


float IMU::getTiltRate()
{
    return tiltRate;
}


float IMU::getSurfaceDisturbanceScore()
{
    return surfaceDisturbanceScore;
}
