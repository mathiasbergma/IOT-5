#include "Sensor.h"
#include "application.h"

// ###################################################################################
/// @brief Sensor Interrupt Service Rutine - run when sensor activates the sensor pin.
// This updates the time since last activation, and the time difference (delta) is
// used to calculate a power reading.
// ###################################################################################
void Sensor::sensorISR(void)
{
    unsigned long timeNow = millis();
    unsigned long delta = timeNow - lastReadTime;
    if (delta > 100)
    {
        currentPowerReading = WATT_CONVERSION_CONSTANT / delta;
        lastReadTime = timeNow;
        newReadingAvaliable = true;
    }
}

// ##############################################################
/// @brief Initialization. Sets up the sensor pin, and interrupt.
// ##############################################################
void Sensor::initSensor()
{
    pinMode(KW_SENSOR_PIN, INPUT_PULLDOWN);                           // Setup pinmode for LDR pin
    attachInterrupt(KW_SENSOR_PIN, &Sensor::sensorISR, this, RISING); // Attach interrup that will be called when rising
    lastReadTime = millis();
    newReadingAvaliable = false;
}

// ###############################################################
/// @brief Method for getting the current calculated power reading
// ###############################################################
int Sensor::getCurrentReading()
{
    newReadingAvaliable = false;
    return currentPowerReading;
}

// ##################################################################
/// @brief Method for checking if a new power reading has been taken,
// Since the last one was read.
// ##################################################################
bool Sensor::checkForNewReading()
{
    return newReadingAvaliable;
}