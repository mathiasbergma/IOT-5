#include "Particle.h"
//#include "state_variables.h"

#define MAX_RANGE 24

extern bool SENSOR_READ;
extern bool GET_DATA;
extern volatile bool CALCULATE;
extern bool TRANSMIT_PRICE;
extern bool TRANSMIT_SENSOR;
extern bool STANDBY_STATE;
extern bool AWAITING_DATA;
extern bool STARTUP;
extern bool ROTATE;
extern bool UPDATE_WH_TODAY;

char temp[2 * 513];    // Create an array that can hold the entire transmission
double * cost_yesterday;
double * cost_today;
double * cost_tomorrow;

const struct transport_t
{
    double high = 1.9135;
    double medium = 0.6379;
    double low = 0.2127;
}transport;

/** @brief Puplishes a formatted command string to Particle cloud that fires off a webhook
 *  @param day
 */
void get_data(int day)
{
    
    temp[0] = 0;
    String data = String::format("{ \"year\": \"%d\", \"month\":\"%02d\", \"day\": \"%02d\"}", Time.year(), Time.month(), day);

    // Trigger the integration
    Particle.publish("elpriser", data, PRIVATE);
}

void myHandler(const char *event, const char *data)
{
    bool populate = false; // Entire transmission received flag

    /* When transmissions are greater than 512 bytes, it will be split into 512
     * byte parts. The final transmission part should therefore be less than 512.
     * Save transmission size into variable so we can act on it
     */
    int transmission_size = strlen(data);

    // "eventname/<transmission part no>"
    char event_str[12];
    strcpy(event_str, event);
    //Serial.printf("event_str: %s\n", event_str);
    // Token used for strtok()
    char *token = NULL;
    Serial.println("in the callback");
    // Extract the numbered part of eventname and use it for indexing "temp"
    strcat(&temp[atoi(strtok(event_str, "prices/")) * 512], data);
    // If transmission size is less than 512 = last transmission received
    if (transmission_size < 512)
    {
        populate = true;
    }

    if (populate)
    {
        // Display what has been received
        Serial.printf("%s\n", temp);

        // Tokenize the string. i.e. split the string so we can get to the data
        token = strtok(temp, ",!");
        for (int i = 0; i < MAX_RANGE; i++)
        {
            // Save hour and cost in differen containers
            //sscanf(token, "%*d-%*d-%*dT%d:%*d:%*d", i);
            if (token == NULL)
            {
                break;
            }
            if (i >= 0 && i < 7)
            {
                 cost_tomorrow[i] = (atof(token) / 1000.0)+transport.low;
            }
            else if (i > 16 && i < 22)
            {
                cost_tomorrow[i] = (atof(token) / 1000.0)+transport.high;
            }
            else
            {
                cost_tomorrow[i] = (atof(token) / 1000.0)+transport.medium;
            }
            token = strtok(NULL, ",!");

        }
        CALCULATE = true;
    }
}