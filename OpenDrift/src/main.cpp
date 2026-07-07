#include <Arduino.h>
#include <Wire.h>
#include "SensorQMI8658.hpp"

#define I2C_SDA 6
#define I2C_SCL 7

SensorQMI8658 qmi;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("OpenDrift Starting...");

    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, I2C_SDA, I2C_SCL))
    {
        Serial.println("QMI8658 NOT detected!");

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println("QMI8658 detected!");

    // Configure accelerometer
    qmi.configAccelerometer(
        SensorQMI8658::ACC_RANGE_4G,
        SensorQMI8658::ACC_ODR_1000Hz,
        SensorQMI8658::LPF_MODE_0
    );

    // Configure gyro
    qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_1024DPS,
        SensorQMI8658::GYR_ODR_896_8Hz,
        SensorQMI8658::LPF_MODE_0
    );

    qmi.enableAccelerometer();
    qmi.enableGyroscope();

    Serial.println("IMU ready!");
}


void loop()
{
    float gx;
    float gy;
    float gz;

    float ax;
    float ay;
    float az;

    qmi.getGyroscope(gx, gy, gz);
    qmi.getAccelerometer(ax, ay, az);

    Serial.println("--------------------");

    Serial.print("Gyro X: ");
    Serial.print(gx);
    Serial.print("  Y: ");
    Serial.print(gy);
    Serial.print("  Z: ");
    Serial.println(gz);

    Serial.print("Accel X: ");
    Serial.print(ax);
    Serial.print("  Y: ");
    Serial.print(ay);
    Serial.print("  Z: ");
    Serial.println(az);

    delay(100);
}