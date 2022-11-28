#include "../lib/MQTT/src/MQTT.h"
#include "BLE_include.h"
#include "application.h"
#include "cost_calc.h"
#include "mDNSResolver.h"
#include "price_handler.h"
#include "state_variables.h"
#include "string.h"
#include "update_json.h"
#include <fcntl.h>
#include "../lib/Json/src/Arduino_JSON.h"
#include "prices.h" //experimental price getting
#include "ppp.h"
//#define STATEDEBUG

//#define USEMQTT

// use rising sensor
#define RISING_SENSOR

#define KW_SENSOR_PIN D8
#define WATT_CONVERSION_CONSTANT 3600000
#define HOST "192.168.0.103"
#define PORT 1883
#define PULL_TIME_1 13
#define PULL_TIME_2 0

#define MAX_TRANSMIT_BUFF 128
#define SLEEP_DURATION 30000

int oneShotGuard = -1;
int oneShotGuard2 = -1;
int oneShotGuard3 = -1;
int *wh_yesterday;
int *wh_today;

int fd_today;
int fd_yesterday;
int fd_wh_today;
int fd_wh_yesterday;

uint8_t currentHour = Time.hour();

bool NewBLEConnection = false;
int last_connect = 0;
int calc_power;
String pricestoday_Json;
String pricestomorrow_Json;
String pricesyesterday_Json;
String wh_today_Json;
String wh_yesterday_Json;

void timer_callback(void);                                // Timer callback function
void init_memory(void);                                   // Initialize memory for the wh and price arrays
void handle_sensor(void);                                 // Handles the sensor reading and calculation of power
void check_mqtt(void);                                    // Check if MQTT is connected - Reconnect of needed
void transmit_prices(int start_stop[12][2], int cnt);     // Transmits low price intervals to MQTT
void myPriceHandler(const char *event, const char *data); // Handler for webhook

Timer timer(60000, check_time, true); // One-shot timer.

#ifdef USEMQTT
// Callback function for MQTT transmission
void callback(char *topic, byte *payload, unsigned int length);
// Create MQTT client
MQTT client("192.168.110.6", PORT, 512, 30, callback);
#endif

// UDP udp;
// mDNSResolver::Resolver resolver(udp);

SYSTEM_THREAD(ENABLED);

void setup()
{
    STARTUP = true;

    waitUntil(Particle.connected);
    Particle.unsubscribe();

    // setup BLE
    ble_setup();

    // Initialize memory
    init_memory();

    Time.zone(1);
    // Time.beginDST();

#ifdef USEMQTT
    // Resolve MQTT broker IP address
    IPAddress IP = resolver.search("homeassistant.local");
    client.setBroker(IP.toString(), PORT);

#endif

    // Subscribe to the integration response event
    Particle.subscribe("prices", myHandler);
    delay(10000);

#ifdef USEMQTT
    // connect to the mqtt broker(unique id by Time.now())
    Serial.printf("Return value: %d", client.connect("client_" + String(Time.now()), "mqtt", "mqtt"));

    // publish/subscribe
    if (client.isConnected())
    {
        Serial.printf("Connected to MQTT broker\n");
        // Debugging publish
        client.publish("power/get", "hello world");
        // Subscribe to 2 topics
        // client.subscribe("power/get");
        client.subscribe("power/prices");
    }
#endif

    // Print current time
    Serial.printf("Current HH:MM: %02d:%02d\n", Time.hour(), Time.minute());
    timer.start();

    Serial.printlnf("RSSI=%d", (int8_t)WiFi.RSSI());


    Serial.printf("trying the HTTP GET\n");
    Httprequest_today();

    Serial.println("trying the other method");
    htttttp();

    // Fill price arrays with data from webhook
    Serial.printf("Getting price data for yesterday\n");
    // get_data(Time.day() - 1);
    // delay(10000);
    int count = 0;
    get_data(Time.day() - 1);
    while (!CALCULATE)
    {
        delay(2000);
        Serial.printf("Count1=: %d\n", count);
        count++;
    }
    CALCULATE = false;
    /* Prices have been fetched. New prices are stored in array for tomorrow.
     *  We therefore need to rotate the arrays to get the correct prices for today.
     */
    delay(5000);
    count = 0;
    rotate_prices();

    Serial.printf("Getting price data for today\n");
    get_data(Time.day());
    while (!CALCULATE)
    {
        delay(1000);
        Serial.printf("Count2=: %d\n", count);
        count++;
    }
    rotate_prices();
    delay(5000);

    if (Time.hour() >= PULL_TIME_1)
    {
        CALCULATE = false;
        GET_DATA = true;
    }
    else
    {
        Serial.printf("The prices for tomorrov will be pulled at %d:00\n", PULL_TIME_1);
        CALCULATE = true;
    }

#ifdef RISING_SENSOR
    pinMode(KW_SENSOR_PIN, INPUT_PULLDOWN);                // Setup pinmode for LDR pin
    attachInterrupt(KW_SENSOR_PIN, handle_sensor, RISING); // Attach interrup that will be called when rising
#else
    pinMode(KW_SENSOR_PIN, INPUT_PULLUP); // Setup pinmode for LDR pin
    attachInterrupt(KW_SENSOR_PIN, handle_sensor, FALLING);
#endif
}

void loop()
{
    static int start_stop[12][2] = {0};
    static int cnt = 0;

#ifdef USEMQTT
    check_mqtt();
#endif

    // Is it time to update the prices or has it been requested?
    if (GET_DATA)
    {
        AWAITING_DATA = true;

        get_data(Time.day() + 1);
        GET_DATA = false;
    }

    // Has the prices for today arrived?
    if (CALCULATE)
    {
        update_JSON();
        cnt = calc_low(start_stop, cost_today, MAX_RANGE);
        Serial.printf("Current HH:MM: %02d:%02d\n", Time.hour(), Time.minute());
        TRANSMIT_PRICE = true;
        CALCULATE = false;
    }

    if (TRANSMIT_PRICE)
    {
        transmit_prices(start_stop, cnt);
        STANDBY_STATE = true;
        TRANSMIT_PRICE = false;
    }

    if (TRANSMIT_SENSOR) // Did we receive a request for updated values
    {
        Serial.printf("Received power/get\n");
        // One flash from sensor equals 1 Whr - Add to total
        wh_today[Time.hour()] += 1;
#ifdef USEMQTT
        char values[16];
        sprintf(values, "%d", calc_power);
        client.publish("power", values);
#endif
        char buffer[255];
        sprintf(buffer, "{\"watt\":%d}", calc_power);
        WattCharacteristic.setValue(buffer);

        // WhrTodayCharacteristic.setValue(update_Whr_Today_JSON());

        // state = STANDBY_STATE;
        TRANSMIT_SENSOR = false;
    }

    if (ROTATE)
    {
        rotate_prices();
        ROTATE = false;
        CALCULATE = true;
    }

    if (UPDATE_WH_TODAY)
    {
#ifdef USEMQTT
        char buffer[16];
        if (Time.hour() == 0)
        {
            sprintf(buffer, "%d", wh_yesterday[23]);
        }
        else
        {
            sprintf(buffer, "%d", wh_today[Time.hour() - 1]);
        }
        client.publish("watthour", buffer);
#endif
        hourly_JSON_update();
        UPDATE_WH_TODAY = false;
    }

    if (NewBLEConnection & ((millis() - last_connect) > 3000))
    {
        // send everything relavant on new connect
        // needs a bit og delay to ensure device is ready
        update_JSON();
        char buffer[255];
        sprintf(buffer, "{\"watt\":%d}", calc_power);
        WattCharacteristic.setValue(buffer);
        DkkYesterdayCharacteristic.setValue(pricesyesterday_Json);
        DkkTodayCharacteristic.setValue(pricestoday_Json);       // string Kr/kwhr
        DkkTomorrowCharacteristic.setValue(pricestomorrow_Json); // string Kr/kwhr
        WhrYesterdayCharacteristic.setValue(wh_yesterday_Json);  // string Whr
        WhrTodayCharacteristic.setValue(wh_today_Json);          // Whr used in the corresponding hour

        DkkTodayCharacteristic.setValue("{\"pricestoday\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}");
        NewBLEConnection = false;
        Serial.printf("ble_connected\n");
    }

    delay(1000);
}

/** @brief Initialize memory. Function is called once at startup. Arrays are rotated afterwards at midnight
 *
 */
void init_memory()
{
    // Serial.printf("before %lu\n", System.freeMemory());
    //  Allocate for the prices
    cost_yesterday = (double *)malloc(MAX_RANGE * sizeof(double));
    if (cost_yesterday == NULL)
    {
        Serial.printf("Failed to allocate memory for cost_yesterday\n");
        while (1)
            ;
    }
    cost_today = (double *)malloc(MAX_RANGE * sizeof(double));
    if (cost_today == NULL)
    {
        Serial.printf("Failed to allocate memory for cost_today\n");
        while (1)
            ;
    }
    cost_tomorrow = (double *)malloc(MAX_RANGE * sizeof(double));
    if (cost_tomorrow == NULL)
    {
        Serial.printf("Failed to allocate memory for cost_tomorrow\n");
        while (1)
            ;
    }
    Serial.printf("Memory allocated for prices: %d bytes of doubles\n", 3 * MAX_RANGE * sizeof(double));
    wh_today = (int *)malloc(MAX_RANGE * sizeof(int));
    if (wh_today == NULL)
    {
        Serial.printf("Failed to allocate memory for wh_today\n");
        while (1)
            ;
    }
    wh_yesterday = (int *)malloc(MAX_RANGE * sizeof(int));
    if (wh_yesterday == NULL)
    {
        Serial.printf("Failed to allocate memory for wh_yesterday\n");
        while (1)
            ;
    }
    Serial.printf("Memory allocated for wh: %d bytes of ints\n", 2 * MAX_RANGE * sizeof(int));
    Serial.printf("After %lu\n", System.freeMemory());
    // Set all values to 0
    memset(cost_yesterday, 0, MAX_RANGE * sizeof(double));
    memset(cost_today, 0, MAX_RANGE * sizeof(double));
    memset(cost_tomorrow, 0, MAX_RANGE * sizeof(double));
    memset(wh_today, 0, MAX_RANGE * sizeof(int));
    memset(wh_yesterday, 0, MAX_RANGE * sizeof(int));

    // Hent indhold af filer, så vi har gemte data i tilfælde af reboot
    // fd_today = open("/sd/prices_today.txt", O_RDWR | O_CREAT);
}
/**
 * @brief      Rotates arrays. Decision is made based on the current time. If it is midnight, the arrays are rotated.
 */
void rotate_prices()
{
    // Rotate prices so that we can use the same array for all days
    double *temp = cost_yesterday;
    cost_yesterday = cost_today;
    cost_today = cost_tomorrow;
    cost_tomorrow = temp;

    memset(cost_tomorrow, 0, MAX_RANGE * sizeof(double));

    int *temp2 = wh_yesterday;
    wh_yesterday = wh_today;
    wh_today = temp2;
    memset(wh_today, 0, MAX_RANGE * sizeof(int));

    // Opdater filer med nye priser
}
/**
 * @brief    Sets a flag when a new BLE connection is established
 */
void BLEOnConnectcallback(const BlePeerDevice &peer, void *context)
{
    NewBLEConnection = true;
    last_connect = millis();
}
/**
 * @brief    IRQ handler for the KW sensor. This function is called every time the KW sensor detects a pulse.
 */
void handle_sensor(void)
{
    static unsigned long last_read = 0;
    unsigned long current_reading = millis();
    unsigned long delta = current_reading - last_read;

    // Check if we have a valid reading. I.e. at least 100 ms since last reading, which is equal to 36kW
    if (delta > 100)
    {
        // We have a valid reading
        calc_power = WATT_CONVERSION_CONSTANT / delta;
        last_read = current_reading;

        // One flash from sensor equals 1 Whr - Add to total
        wh_today[currentHour] += 1;

        // Update flag - Transmit sensor values
        TRANSMIT_SENSOR = true;
    }
}

void callback(char *topic, byte *payload, unsigned int length)
{
    GET_DATA = true;
}

/** @brief Reconnects MQTT client if disconnected
 */
void check_mqtt(void)
{
#ifdef USEMQTT
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
#endif
}

/**
 * @brief      Transmits the time intervals where prices are low to MQTT broker
 */
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
#ifdef USEMQTT
    // Publish cheap hour to MQTT
    client.publish("prices", data);
    client.loop();
#endif
}
/**
 * @brief     Checks the current time and decides if it is time to update the prices, update watt hours or rotate price and watt hour arrays.
 */
void check_time(void)
{
    currentHour = Time.hour();
    uint8_t currentMinute = Time.minute();
    uint8_t currentSecond = Time.second();
    uint8_t currentDay = Time.day();
    uint countdown;

    // Set new countdown to aim for xx:00:01 within a second (+1 for safe side)
    countdown = ((60 - currentMinute) * 60000) - (currentSecond + 1);

    // Start timer again with new countdown
    timer.stop();
    timer.changePeriod(countdown);
    timer.start();

    if ((currentHour == PULL_TIME_1) && currentDay != oneShotGuard)
    {
        oneShotGuard = currentDay;
        GET_DATA = true;
    }
    if ((currentHour == PULL_TIME_2) && currentDay != oneShotGuard2)
    {
        oneShotGuard2 = currentDay;

        ROTATE = true;
    }
    if (currentMinute == 0 && currentHour != oneShotGuard3)
    {
        oneShotGuard3 = currentHour;
        // Update the wh_today array
        UPDATE_WH_TODAY = true;
    }
}