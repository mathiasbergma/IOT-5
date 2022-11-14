#ifndef STATE_VARIABLES_H
#define STATE_VARIABLES_H

enum statemachine
{
    SENSOR_READ = 0,
    GET_DATA = 1,
    CALCULATE = 2,
    TRANSMIT_PRICE = 3,
    TRANSMIT_SENSOR = 4,
    SLEEP_STATE = 5,
    AWAITING_DATA = 6,
    STARTUP = 10,
    ROTATE = 11,
};

#endif