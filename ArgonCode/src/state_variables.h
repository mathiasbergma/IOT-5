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

const struct transport_t
{
    double high = 1.9135;
    double medium = 0.6379;
    double low = 0.2127;
}transport;

char temp[2 * 513];    // Create an array that can hold the entire transmission
double * cost_yesterday;
double * cost_today;
double * cost_tomorrow;

#endif