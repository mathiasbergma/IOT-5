#include "string.h"
#include "application.h"
#include "../lib/MQTT/src/MQTT.h"

#define DELTA_OFFSET 0.3
#define KW_SENSOR_PIN D8
#define WATT_CONVERSION_CONSTANT 3600000
#define HOST "192.168.0.103"
#define PORT 1883
#define SECONDS_TO_DAY 86400
#define PULL_TIME_1 23
#define PULL_TIME_2 11
#define MAX_RANGE 48
#define MAX_TRANSMIT_BUFF 128
#define SLEEP_DURATION 30000

int oneShotGuard = -1;
double cost[MAX_RANGE];
int cost_hour[MAX_RANGE];
int date;
int range = MAX_RANGE;         // Max received count. Updated if received count is smaller
char temp[5*513];       // Create an array that can hold the entire transmission
bool populate = false;  // Entire transmission received flag
bool work = false;      // Received price data
bool printer = false;
bool update_prices = false;

int low_range_hour[24];

int cnt;
int start_stop[12][2] = {0};

unsigned long last_read;
int calc_power;

// Send updated meassurement flag
bool transmit_value = false;

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
};

statemachine state = GET_DATA;

void calc_low(void);
void get_data(int day);
void handle_sensor(void);
void reconnect(void);

// Callback function for MQTT transmission
void callback(char* topic, byte* payload, unsigned int length);
// Create MQTT client
MQTT client("192.168.0.103", PORT, 512, 30, callback);
// Create sleep instance
SystemSleepConfiguration config;

SYSTEM_THREAD(ENABLED);
//SYSTEM_MODE(MANUAL);

void setup()
{
    //Particle.connect();
    pinMode(SENSOR_READ, OUTPUT);
    pinMode(GET_DATA, OUTPUT);
    pinMode(CALCULATE, OUTPUT);
    pinMode(TRANSMIT_PRICE, OUTPUT);
    pinMode(TRANSMIT_SENSOR, OUTPUT);
    pinMode(SLEEP_STATE, OUTPUT);
    pinMode(AWAITING_DATA, OUTPUT);
    pinMode(STARTUP, OUTPUT);

    digitalWrite(STARTUP, HIGH);
    waitUntil(Particle.connected);
    digitalWrite(STARTUP, LOW);

    digitalWrite(state, HIGH);
    last_read = millis(); //Give it an initial value
    pinMode(KW_SENSOR_PIN, INPUT_PULLDOWN);                 //Setup pinmode for LDR pin
    attachInterrupt(KW_SENSOR_PIN,handle_sensor,RISING);    //Attach interrup that will be called when rising
    
    // Subscribe to the integration response event
    Particle.subscribe("prices", myHandler, MY_DEVICES);
    Particle.subscribe("get_prices", myPriceHandler, MY_DEVICES);

    // Request data on power prices for the next 48 hours
    //get_data(Time.day());
    Particle.variable("State",state);

    // connect to the server(unique id by Time.now())
    Serial.printf("Return value: %d",client.connect("sparkclient_" + String(Time.now()),"mqtt","mqtt"));

    // publish/subscribe
    if (client.isConnected()) 
    {
        // Debugging publish
        client.publish("power/get","hello world");
        // Subscribe to 2 topics
        //client.subscribe("power/get");
        client.subscribe("power/prices");
    }

    // Setup low power mode
    //config.mode(SystemSleepMode::ULTRA_LOW_POWER).gpio(KW_SENSOR_PIN, RISING).network(NETWORK_INTERFACE_ALL);
}

void callback(char* topic, byte* payload, unsigned int length) 
{
    /*
    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = NULL;
    */

    work = true;
}

void handle_sensor(void)
{

    //statemachine prev_state = state;
    unsigned long delta;
    unsigned long current_reading = millis();

    if ((delta = current_reading-last_read) > 100)
    {
        Serial.printf("In interrupt\n");
        digitalWrite(state, LOW);
        state = SENSOR_READ;
        digitalWrite(state, HIGH);
        calc_power = WATT_CONVERSION_CONSTANT / delta;
        last_read = current_reading;
        transmit_value = true;
        digitalWrite(state, LOW);
        state = TRANSMIT_SENSOR;
        //state = prev_state;
        digitalWrite(state, HIGH);
    }
}

void myHandler(const char *event, const char *data)
{
    populate = false;

    /* When transmissions are greater than 512 bytes, it will be split into 512
     * byte parts. The final transmission part should therefore be less than 512.
     * Save transmission size into variable so we can act on it
    */
    int transmission_size = strlen(data);
    
    // "eventname/<transmission part no>"
    char event_str[12];
    strcpy(event_str,event);

    // Token used for strtok()
    char *token = NULL;
    // Extract the numbered part of eventname and use it for indexing "temp"
    strcat(&temp[atoi(strtok(event_str,"prices/"))*512],data);
    // If transmission size is less than 512 = last transmission received
    if (transmission_size < 512)
    {
        populate = true;
    }

    if (populate)
    {
        // Display what has been received
        Serial.printf("%s\n",temp);
        
        // Tokenize the string. i.e. split the string so we can get to the data
        token = strtok(temp, ",!");
        for (int i = 0; i < range; i++)
        {
            // Save hour and cost in differen containers
            sscanf(token, "%*d-%*d-%dT%d:%*d:%*d", &date, &cost_hour[i]);
            token = strtok(NULL, ",!");
            cost[i] = atof(token) / 1000;
            
            if((token = strtok(NULL, ",!")) == NULL) // Received data count is less than 48.
            {
                range = i;  // Update range, such that the rest of program flow is aware of size
                break;      // Break the while loop
            }
        }
        digitalWrite(state, LOW);
        state = CALCULATE;
        digitalWrite(state, HIGH);
    }
}

void myPriceHandler(const char *event, const char *data)
{
    update_prices = true;
    digitalWrite(state, LOW);
    state = GET_DATA;
    digitalWrite(state, HIGH);
}

void loop()
{
    // Note: Change to be hour based so device can sleep longer
    // wakeup on interrupt but go back to sleep again.
    // Check what time it is
    int currentHour = Time.hour();
    if (( currentHour == PULL_TIME_1 || currentHour == PULL_TIME_2) && currentHour != oneShotGuard)
    {
        oneShotGuard = currentHour;
        update_prices = true;
        digitalWrite(state, LOW);
        state = GET_DATA;
        digitalWrite(state, HIGH);
    }
    
    if (client.isConnected())
    {
        client.loop();
    }
    else 
    {
        Serial.printf("Client disconnected\n");
        reconnect();
        if (client.isConnected())
        {
            Serial.printf("Client reconnected\n");
        }
    }
    

    // Is it time to update the prices or has it been requested?
    if (state == GET_DATA)
    {
        digitalWrite(state, LOW);
        state = AWAITING_DATA;
        digitalWrite(state, HIGH);
        get_data(Time.day());
    }
    
    // Has the prices for today arrived?
    if (state == CALCULATE)
    {
        calc_low();
        Serial.printf("Current HH:MM: %02d:%02d\n", Time.hour() + 2, Time.minute());
    }

    if (state == TRANSMIT_PRICE)
    {
        
        Serial.printf("In work\n");
        // Do some meaningfull work with the collected data
        String data = "Cheap(ish) hours of the day: ";
        for (int z = 0; z < cnt; z++)
        {
            data += String::format("%02d to %02d, ",start_stop[z][0],start_stop[z][1]);
        }
        // Publish the cheap hours to cloud
        Particle.publish("Low price hours", data, PRIVATE);
        // Publish cheap hour to MQTT
        client.publish("prices", data);
        client.loop();
        work = false;
        digitalWrite(state, LOW);
        state = SLEEP_STATE;
        digitalWrite(state, HIGH);
        
    }

    if (state == TRANSMIT_SENSOR) // Did we receive a request for updated values
    {
        Serial.printf("Received power/get\n");
        char values[16];
        sprintf(values,"%d", calc_power);
        client.publish("power",values);
        transmit_value = false;
        digitalWrite(state, LOW);
        state = SLEEP_STATE;
        digitalWrite(state, HIGH);
    }
    // Wait 1 second
    delay(1000);

    /*
    // Get current millis()
    system_tick_t current_reading = millis();
    // Dynamic sleep duration 
    system_tick_t elapsed_sleep;
    if (state == SLEEP_STATE)
    {
        
        while ((elapsed_sleep = millis() - current_reading) < SLEEP_DURATION)
        {
            client.disconnect();
            Particle.disconnect();
            WiFi.off();

            Serial.printf("In sleep loop: %ld\n",SLEEP_DURATION-elapsed_sleep);
            config.mode(SystemSleepMode::ULTRA_LOW_POWER).duration(SLEEP_DURATION-elapsed_sleep).gpio(KW_SENSOR_PIN, RISING);
            System.sleep(config);
        }
        /*
        if (Particle.connected())
        {
            Particle.process();
        }
        */
        // Roughly 10 seconds have passed!
        /*
        digitalWrite(state, LOW);
        state = TRANSMIT_SENSOR;
        digitalWrite(state, HIGH);
        WiFi.on();
        WiFi.connect();
        waitUntil(WiFi.ready);
        Particle.connect();
        Particle.process();
    }
    */
}

/** @brief Reconnects MQTT client if disconnected
 */
void reconnect(void)
{
    client.connect("sparkclient_" + String(Time.now()),"mqtt","mqtt");
}

/** @brief The purpose of the function is to identify the hours at which the highest and lowest cost are.
 *  Furthermore neighbouring low cost hour are identified and saved in an array for easy presentation
*/
void calc_low(void)
{
    int idx = 0;

    double delta;
    double small_offset;
    double last_big = 0;
    double last_small = 100; // Assign any absurdly high value

    for (int i = 0; i < range; i++)
    {
        // Find the highest price in range
        if (cost[i] > last_big)
        {
            last_big = cost[i];
        }
        // Find the lowest price in range
        if (cost[i] < last_small)
        {
            last_small = cost[i];
        }
    }
    // Calculate delta
    delta = last_big - last_small;

    // Define low price area
    small_offset = last_small + delta * DELTA_OFFSET;
    
    // Find hours of day at which price is within the defined low price point
    for (int i = 0; i <= range; i++)
    {
        
        if (cost[i] < small_offset)
        {
            low_range_hour[idx] = cost_hour[i];
            
            idx++;
        }
    }

    // Display the results
    Serial.printf("Highest price of the day: %f\n", last_big);
    Serial.printf("Lowest price of the day: %f\n", last_small);
    Serial.printf("Hours of the day where electricity is within accepted range:\n");
    
    int i = 0;
    if (idx > 0)
    {
        while (i <= idx)
        {
            start_stop[cnt][0] = low_range_hour[i];

            while (low_range_hour[i] == low_range_hour[i + 1] - 1) // Hour only increased by 1. I.e. coherant
            {
                i++;
            }
            
            start_stop[cnt][1] = low_range_hour[i]+1;
            
            cnt++;
            i++;
        }
        cnt--;
    }
    for (int z = 0; z < cnt; z++)
    {
        Serial.printf("%02d to %02d\n",start_stop[z][0],start_stop[z][1]);
    }

    // Calculations are done - set state
    work = true;
    digitalWrite(state, LOW);
    state = TRANSMIT_PRICE;
    digitalWrite(state, HIGH);
}

/** @brief Puplishes a formatted command string to Particle cloud that fires off a webhook
 *  @param day
 */
void get_data(int day)
{
    range = MAX_RANGE;
    cnt = 0;
    temp[0] = 0;
    String data = String::format("{ \"year\": \"%d\", \"month\":\"%02d\", \"day\": \"%02d\", \"day_two\": \"%02d\", \"hour\": \"%02d\" }", Time.year(), Time.month(), day, day + 2, Time.hour());
    
    // Trigger the integration
    Particle.publish("elpriser", data, PRIVATE);
}