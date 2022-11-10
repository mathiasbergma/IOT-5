#include "string.h"
#include "application.h"
#include "../lib/MQTT/src/MQTT.h"
#include "cost_calc.h"
#include "state_variables.h"
#include "mDNSResolver.h"
#include "BLE_include.h"


//#define STATEDEBUG 1

#define KW_SENSOR_PIN D8
#define WATT_CONVERSION_CONSTANT 3600000
#define HOST "192.168.0.103"
#define PORT 1883
#define PULL_TIME_1 23
#define PULL_TIME_2 11
#define MAX_RANGE 48
#define MAX_TRANSMIT_BUFF 128
#define SLEEP_DURATION 30000

statemachine state;

int oneShotGuard = -1;
double cost[MAX_RANGE];
int cost_hour[MAX_RANGE];
int range = MAX_RANGE; // Max received count. Updated if received count is smaller
char temp[5 * 513];    // Create an array that can hold the entire transmission
bool NewBLEConnection =  false;
int last_connect = 0;
int calc_power;

const struct transport_t
{
    double high = 1.9135;
    double medium = 0.6379;
    double low = 0.2127;
}transport;

//int calc_low(int **low_price_intervals);
void get_data(int day);
void handle_sensor(void);
void check_mqtt(void);
void init_GPIO(void);
void transmit_prices(int start_stop[12][2], int cnt);
void handle_sensor(void);
void myHandler(const char *event, const char *data);
void myPriceHandler(const char *event, const char *data);

// Callback function for MQTT transmission
void callback(char *topic, byte *payload, unsigned int length);
// Create MQTT client
MQTT client("192.168.110.6", PORT, 512, 30, callback);
// Create sleep instance
SystemSleepConfiguration config;

UDP udp;
mDNSResolver::Resolver resolver(udp);

SYSTEM_THREAD(ENABLED);

void setup()
{
    // Particle.connect();
    init_GPIO();

    // setup BLE
    ble_setup();

    state = STARTUP;
#ifdef STATEDEBUG
    digitalWrite(state, LOW);
#endif
    waitUntil(Particle.connected);
#ifdef STATEDEBUG
    digitalWrite(state, HIGH);
#endif
    state = GET_DATA;
#ifdef STATEDEBUG
    digitalWrite(state, HIGH);
#endif
    
    pinMode(KW_SENSOR_PIN, INPUT_PULLDOWN);                // Setup pinmode for LDR pin
    attachInterrupt(KW_SENSOR_PIN, handle_sensor, RISING); // Attach interrup that will be called when rising

    IPAddress IP = resolver.search("homeassistant.local");
    
    Particle.publish("HA IP", IP.toString());

    client.setBroker(IP.toString(), PORT);

    // Subscribe to the integration response event
    Particle.subscribe("prices", myHandler, MY_DEVICES);
    Particle.subscribe("get_prices", myPriceHandler, MY_DEVICES);

    // Publish state variable to Particle cloud
    Particle.variable("State", state);

    // connect to the mqtt broker(unique id by Time.now())
    Serial.printf("Return value: %d", client.connect("client_" + String(Time.now()), "mqtt", "mqtt"));

    // publish/subscribe
    if (client.isConnected())
    {
        // Debugging publish
        client.publish("power/get", "hello world");
        // Subscribe to 2 topics
        // client.subscribe("power/get");
        client.subscribe("power/prices");
    }

    // Setup low power mode
    // config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio(KW_SENSOR_PIN, RISING).network(NETWORK_INTERFACE_ALL);
}

void loop()
{
    static int start_stop[12][2] = {0};
    static int cnt = 0;
    // Note: Change to be hour based so device can sleep longer
    // wakeup on interrupt but go back to sleep again.
    // Check what time it is
    check_time();

    //check_mqtt();

    // Is it time to update the prices or has it been requested?
    if (state == GET_DATA)
    {
#ifdef STATEDEBUG
        digitalWrite(state, LOW);
#endif
        state = AWAITING_DATA;
#ifdef STATEDEBUG
        digitalWrite(state, HIGH);
#endif
        get_data(Time.day());
    }

    // Has the prices for today arrived?
    if (state == CALCULATE)
    {
        cnt = calc_low(start_stop, cost, cost_hour, range);
        Serial.printf("Current HH:MM: %02d:%02d\n", Time.hour() + 2, Time.minute());
    }

    if (state == TRANSMIT_PRICE)
    {
        transmit_prices(start_stop, cnt);
    }

    if (state == TRANSMIT_SENSOR) // Did we receive a request for updated values
    {
        Serial.printf("Received power/get\n");
        char values[16];
        sprintf(values, "%d", calc_power);
        client.publish("power", values);
#ifdef STATEDEBUG
        digitalWrite(state, LOW);
#endif
        state = SLEEP_STATE;
#ifdef STATEDEBUG
        digitalWrite(state, HIGH);
#endif
    }

    if(NewBLEConnection & ((millis()-last_connect)>1400)){
        //send everything relavant on new connect
        // needs a bit og delay to ensure device is ready
        //WattCharacteristic.setValue(300,1);   // to send int value
        
        DkkTodayCharacteristic.setValue("{\"pricestoday\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,24]}");  // string mKr/kwhr
        DkkTomorrowCharacteristic.setValue("{\"pricestomorrow\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}"); // string mKr/kwhr
        WhrTodayCharacteristic.setValue("{\"WHr_today\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}"); // Whr used in the corrisponding hour
        NewBLEConnection = false;
        Serial.printf("ble_connected");
    }

}

void BLEOnConnectcallback(const BlePeerDevice& peer, void* context){
    NewBLEConnection = true;
    last_connect = millis();
}

void handle_sensor(void)
{
    static unsigned long last_read = 0;
    // statemachine prev_state = state;
    unsigned long delta;
    unsigned long current_reading = millis();

    if ((delta = current_reading - last_read) > 100)
    {
        Serial.printf("In interrupt\n");
#ifdef STATEDEBUG
        digitalWrite(state, LOW);
#endif
        state = SENSOR_READ;
#ifdef STATEDEBUG
        digitalWrite(state, HIGH);
#endif
        calc_power = WATT_CONVERSION_CONSTANT / delta;
        last_read = current_reading;
#ifdef STATEDEBUG
        digitalWrite(state, LOW);
#endif
        state = TRANSMIT_SENSOR;
// state = prev_state;
#ifdef STATEDEBUG
        digitalWrite(state, HIGH);
#endif
    }
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

    // Token used for strtok()
    char *token = NULL;
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
        for (int i = 0; i < range; i++)
        {
            // Save hour and cost in differen containers
            sscanf(token, "%*d-%*d-%*dT%d:%*d:%*d", &cost_hour[i]);
            token = strtok(NULL, ",!");
            if (cost_hour[i] >= 0 && cost_hour[i] < 7)
            {
                 cost[i] = (atof(token) / 1000)+transport.low;
            }
            else if (cost_hour[i] > 16 && cost_hour[i] < 22)
            {
                cost[i] = (atof(token) / 1000)+transport.high;
            }
            else
            {
                cost[i] = (atof(token) / 1000)+transport.medium;
            }

            if ((token = strtok(NULL, ",!")) == NULL) // Received data count is less than 48.
            {
                range = i; // Update range, such that the rest of program flow is aware of size
                break;     // Break the while loop
            }
        }
#ifdef STATEDEBUG
        digitalWrite(state, LOW);
#endif
        state = CALCULATE;
#ifdef STATEDEBUG
        digitalWrite(state, HIGH);
#endif
    }
}

void myPriceHandler(const char *event, const char *data)
{
#ifdef STATEDEBUG
    digitalWrite(state, LOW);
#endif
    state = GET_DATA;
#ifdef STATEDEBUG
    digitalWrite(state, HIGH);
#endif
}

void init_GPIO(void)
{
    pinMode(SENSOR_READ, OUTPUT);
    pinMode(GET_DATA, OUTPUT);
    pinMode(CALCULATE, OUTPUT);
    pinMode(TRANSMIT_PRICE, OUTPUT);
    pinMode(TRANSMIT_SENSOR, OUTPUT);
    pinMode(SLEEP_STATE, OUTPUT);
    pinMode(AWAITING_DATA, OUTPUT);
    pinMode(STARTUP, OUTPUT);
}

void callback(char *topic, byte *payload, unsigned int length)
{
#ifdef STATEDEBUG
    digitalWrite(state, LOW);
#endif
    state = GET_DATA;
#ifdef STATEDEBUG
    digitalWrite(state, HIGH);
#endif
}


/** @brief Reconnects MQTT client if disconnected
 */
void check_mqtt(void)
{
    if (client.isConnected())
    {
        client.loop();
    }
    else
    {
        Serial.printf("Client disconnected\n");
        client.connect("sparkclient_" + String(Time.now()), "mqtt", "mqtt");
        if (client.isConnected())
        {
            Serial.printf("Client reconnected\n");
        }
    }
}

/** @brief Puplishes a formatted command string to Particle cloud that fires off a webhook
 *  @param day
 */
void get_data(int day)
{
    range = MAX_RANGE;
    temp[0] = 0;
    String data = String::format("{ \"year\": \"%d\", \"month\":\"%02d\", \"day\": \"%02d\", \"day_two\": \"%02d\", \"hour\": \"%02d\" }", Time.year(), Time.month(), day, day + 2, Time.hour());

    // Trigger the integration
    Particle.publish("elpriser", data, PRIVATE);
}
void transmit_prices(int start_stop[12][2], int size)
{
    Serial.printf("In work\n");
    // Do some meaningfull work with the collected data
    String data = "Cheap(ish) hours of the day: ";
    for (int z = 0; z < size; z++)
    {
        data += String::format("%02d to %02d, ", start_stop[z][0], start_stop[z][1]);
    }
    // Publish the cheap hours to cloud
    Particle.publish("Low price hours", data, PRIVATE);
    // Publish cheap hour to MQTT
    client.publish("prices", data);
    client.loop();
#ifdef STATEDEBUG
    digitalWrite(state, LOW);
#endif
    state = SLEEP_STATE;
#ifdef STATEDEBUG
    digitalWrite(state, HIGH);
#endif
}
void check_time(void)
{
    int currentHour = Time.hour();
    if ((currentHour == PULL_TIME_1 || currentHour == PULL_TIME_2) && currentHour != oneShotGuard)
    {
        oneShotGuard = currentHour;
#ifdef STATEDEBUG
        digitalWrite(state, LOW);
#endif
        state = GET_DATA;
#ifdef STATEDEBUG
        digitalWrite(state, HIGH);
#endif
    }
}