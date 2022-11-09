#pragma once

#include "application.h"

#define KW_SENSOR_PIN D8
#define WATT_CONVERSION_CONSTANT 3600000

class Sensor
{
  private:
    unsigned long lastReadTime;
    int currentPowerReading;
    bool newReadingAvaliable;

    void sensorISR();

  public:
    void initSensor();
    int getCurrentReading();
    bool checkForNewReading();
};
