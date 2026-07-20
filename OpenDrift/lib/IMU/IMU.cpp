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
        return false;
    }


    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_0
    );


    qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_1024DPS,
        SensorQMI8658::GYR_ODR_896_8Hz,
        SensorQMI8658::LPF_MODE_0
    );


    qmi.enableAccelerometer();
    qmi.enableGyroscope();


    return true;
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

    qmi.getGyroscope(
        gyroX,
        gyroY,
        gyroZ
    );

    if(!qmi.getAccelerometer(
        accelX,
        accelY,
        accelZ
    ))
    {
        return;
    }

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

    // Shadow-only terrain detector. This score is logged for validation but
    // deliberately has no influence on steering yet.
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
