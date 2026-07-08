#include "IMU.h"


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
    qmi.getGyroscope(
        gyroX,
        gyroY,
        gyroZ
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