#ifndef STATE_VARIABLES_H
#define STATE_VARIABLES_H

bool SENSOR_READ = false;
bool GET_DATA = false;
volatile bool CALCULATE = false;
bool TRANSMIT_PRICE = false;
bool TRANSMIT_SENSOR = false;
bool STANDBY_STATE = false;
bool AWAITING_DATA = false;
bool STARTUP = false;
bool ROTATE = false;
bool UPDATE_WH_TODAY = false;

#endif