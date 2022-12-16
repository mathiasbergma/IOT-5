#include "BLE_include.h"
#include "application.h"
#include "cost_calc.h"
#include "price_handler.h"
#include "state_variables.h"
#include "string.h"
#include "update_json.h"
#include <fcntl.h>

#include "../lib/Json/src/Arduino_JSON.h"
#include "price_http_get.h"

#include "Storage.h"

#define USEMQTT

#ifdef USEMQTT
#include "../lib/MQTT/src/MQTT.h"
#include "mDNSResolver.h"
#define HOST "homeassistant.local"
#define PORT 1883
#endif

#define KW_SENSOR_PIN D8
#define WATT_CONVERSION_CONSTANT 3600000
#define PULL_TIME_1 13
#define PULL_TIME_2 0
#define MAX_TRANSMIT_BUFF 128
#define SLEEP_DURATION 30000

// Defines for storage
#define WH_TDAY_FILE "/wattHourToday.txt"
#define WH_YSTDAY_FILE "/wattHourYesterday.txt"
#define LASTWRITE_FILE "/lastWrite.txt"
#define WH_TDAY_INITSTRING "{\"WHr_today\":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}"
#define WH_YSTDAY_INITSTRING "{\"WHr_yesterday\":[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]}"
#define WH_TDAY_KEY "WHr_today"
#define WH_YSTDAY_KEY "WHr_yesterday"

int oneShotGuard = -1;
int oneShotGuard2 = -1;
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

void init_memory(void);                                                                // Initialize memory for the wh and price arrays
void handle_sensor(void);                                                              // Handles the sensor reading and calculation of power
void check_mqtt(void);                                                                 // Check if MQTT is connected - Reconnect of needed
void transmit_prices(int start_stop[12][2], int cnt);                                  // Transmits low price intervals to MQTT
void loadArray(int *whArray, String *whJson, String fileName, const char *propString); // Load array info from file.
void initStorage();                                                                    // Initialize Storage files.
void updateFiles();                                                                    // Update data in storage files.

Timer timer(20000, timerCallback, true); // One-shot timer.

#ifdef USEMQTT
// Callback function for MQTT transmission
void callback(char *topic, byte *payload, unsigned int length);
// Create MQTT client
MQTT client("0.0.0.0", PORT, 512, 30, callback);

UDP udp;
mDNSResolver::Resolver resolver(udp);
#endif

SYSTEM_THREAD(ENABLED);

// ############################# SETUP ######################################

void setup()
{
    STARTUP = true;

    // Wait for connection to Particle - this syncs the clock.
    waitUntil(Particle.connected);
    Particle.unsubscribe();

    // setup BLE
    ble_setup();

    // Initialize memory
    init_memory();

    // Set current hour before entering loop
    Time.zone(1);
    currentHour = Time.hour();

    // Subscribe to the integration response event
    // Particle.subscribe("prices", myHandler);

#ifdef USEMQTT
    // Resolve MQTT broker IP address
    IPAddress IP = resolver.search(HOST);
    client.setBroker(IP.toString(), PORT);

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

    // Print wifi signal strength.
    Serial.printlnf("RSSI=%d", (int8_t)WiFi.RSSI());

    // Fill price arrays with data from http get
    Serial.printf("Getting price data for yesterday\n");
    get_data_http(Time.day() - 1);
    rotate_prices();

    Serial.printf("Getting price data for today\n");
    get_data_http(Time.day());
    rotate_prices();

    // Set up persistant storage
    initStorage();

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

    // Set up sensor pin and interrupt.
    pinMode(KW_SENSOR_PIN, INPUT_PULLDOWN);
    attachInterrupt(KW_SENSOR_PIN, handle_sensor, RISING);

    // Turn off wifi to save power.
#ifndef USEMQTT
        Particle.disconnect();
        WiFi.off();
#endif

    // Start the timer.
    timer.start();
}

// ############################# MAIN LOOP ######################################

void loop()
{
    static int start_stop[12][2] = {0};
    static int cnt = 0;

#ifdef USEMQTT
    check_mqtt();
#endif

    // Check if we wish to fetch prices.
    if (GET_DATA)
    {
        AWAITING_DATA = true;
#ifndef USEMQTT
        WiFi.on();
        Particle.connect();
#endif
        get_data_http(Time.day() + 1);

#ifndef USEMQTT
        Particle.disconnect();
        WiFi.off();
#endif
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

    // Check if prices should be sent (MQTT/Particle cloud)
    if (TRANSMIT_PRICE)
    {
#ifndef USEMQTT
        WiFi.on();
        Particle.connect();
#endif
        transmit_prices(start_stop, cnt);
        STANDBY_STATE = true;
        TRANSMIT_PRICE = false;
#ifndef USEMQTT
        Particle.disconnect();
        WiFi.off();
#endif
    }

    // Sensor ISR was fired.
    if (TRANSMIT_SENSOR)
    {
        char buffer[255];
        sprintf(buffer, "{\"watt\":%d}", calc_power);
        WattCharacteristic.setValue(buffer);
        WhrTodayCharacteristic.setValue(update_Whr_Today_JSON());
        TRANSMIT_SENSOR = false;

#ifdef USEMQTT
        char values[16];
        sprintf(values, "%d", calc_power);
        client.publish("power", values);
#endif
    }

    // Check rotate flag, to see if today is now yesterday.
    if (ROTATE)
    {
        rotate_prices();
        ROTATE = false;
        CALCULATE = true;
    }

    // Periodic updates (on timer reset)
    if (UPDATE_WH_TODAY)
    {
#ifdef USEMQTT
        char buffer[16];
        double total_kwh = 0;
        char kwh_buffer[16];
        if (Time.hour() == 0)
        {
            sprintf(buffer, "%d", wh_yesterday[23]);
            for (int i = 0; i < 24; i++)
            {
                total_kwh += wh_yesterday[i];
            }
        }
        else
        {
            sprintf(buffer, "%d", wh_today[Time.hour() - 1]);
            for (int i = 0; i < Time.hour(); i++)
            {
                total_kwh += wh_today[i];
            }
        }
        sprintf(kwh_buffer, "%.1f", total_kwh / 1000);
        client.publish("watthour", buffer);
        client.publish("total_kwh", kwh_buffer);
#endif
        hourly_JSON_update();
        updateFiles();
        UPDATE_WH_TODAY = false;
    }

    if (NewBLEConnection & ((millis() - last_connect) > 3000))
    {
        // send everything relavant on new connect
        // needs a bit of delay to ensure device is ready
        update_JSON();
        char buffer[255];
        sprintf(buffer, "{\"watt\":%d}", calc_power);
        WattCharacteristic.setValue(buffer);
        DkkYesterdayCharacteristic.setValue(pricesyesterday_Json);
        DkkTodayCharacteristic.setValue(pricestoday_Json);       // string Kr/kwhr
        DkkTomorrowCharacteristic.setValue(pricestomorrow_Json); // string Kr/kwhr
        WhrYesterdayCharacteristic.setValue(wh_yesterday_Json);  // string Whr
        WhrTodayCharacteristic.setValue(wh_today_Json);          // Whr used in the corresponding hour

        NewBLEConnection = false;
        Serial.printf("ble_connected\n");
    }

    delay(1000);
}

// ############################# Function implementations ######################################

/** @brief Initialize memory. Function is called once at startup.
 *         Arrays are rotated afterwards at midnight
 */
void init_memory()
{
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
}

/**
 * @brief   Rotates arrays. Decision is made based on the current time.
 *          If it is midnight, the arrays are rotated.
 */
void rotate_prices()
{
    double *temp = cost_yesterday;
    cost_yesterday = cost_today;
    cost_today = cost_tomorrow;
    cost_tomorrow = temp;
    memset(cost_tomorrow, 0, MAX_RANGE * sizeof(double));

    int *temp2 = wh_yesterday;
    wh_yesterday = wh_today;
    wh_today = temp2;
    memset(wh_today, 0, MAX_RANGE * sizeof(int));
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
 * @brief    IRQ handler for the KW sensor.
 *
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
    // Do some meaningfull work with the collected data
    String data = "Cheap(ish) hours of the day: ";
    for (int z = 0; z < size; z++)
    {
        data += String::format("%02d to %02d, ", start_stop[z][0], start_stop[z][1]);
    }

    // Publish the cheap hours to cloud
    // Particle.publish("Low price hours", data, PRIVATE);

#ifdef USEMQTT
    // Publish cheap hour to MQTT
    client.publish("prices", data);
    client.loop();
#endif
}

/**
 * @brief Timer callback.
 *        Sets new countdown to next quater-Hour.
 *        Also sets flags to update the prices & watt hours or rotate price and watt hour arrays.
 */
void timerCallback(void)
{

    String timeString = Time.format(Time.now(), "%X-%j");
    currentHour = timeString.substring(0, 2).toInt();
    uint8_t currentMinute = timeString.substring(3, 5).toInt();
    uint8_t currentSecond = timeString.substring(6, 8).toInt();
    uint16_t currentDay = timeString.substring(9).toInt();

    // Set new countdown. Aim for next quater + 1 sec for safety.
    uint countdown = (15 * 60000) - ((currentMinute % 15) * 60000) - (currentSecond * 1000) + 1000;

    // Start timer again with new countdown
    timer.stop();
    timer.changePeriod(countdown);
    timer.start();

    // Check if it's time to fetch new prices.
    if ((currentHour == PULL_TIME_1) && currentDay != oneShotGuard)
    {
        oneShotGuard = currentDay;
        GET_DATA = true;
    }

    // Check if it's time to rotate arrays.
    if ((currentHour == PULL_TIME_2) && currentDay != oneShotGuard2)
    {
        oneShotGuard2 = currentDay;
        ROTATE = true;
    }

    // Check if we should update arrays. (every quater-hour)
    if (currentMinute % 5 == 0) //(currentMinute % 15 == 0)
    {
        UPDATE_WH_TODAY = true;
    }
}

/**
 * @brief Initializes files, creating them if they do not yet exist.
 *        Also updates the watt hour Jsons and int arrays.
 */
void initStorage()
{
    Serial.println("######### Initializing storage #########");

    int lastWriteExist = initFile(LASTWRITE_FILE, "000");
    int todayInit = initFile(WH_TDAY_FILE, WH_TDAY_INITSTRING);
    int yesterdayInit = initFile(WH_YSTDAY_FILE, WH_YSTDAY_INITSTRING);

    // Check if lastWrite.txt already existed
    if (lastWriteExist > 0)
    {
        // lastWrite file exists. Initialize watt-hour files
        if (todayInit > -1 && yesterdayInit > -1)
        {
            if (Time.isValid())
            {
                uint16_t dayNow = Time.format(Time.now(), "%j").toInt();
                uint16_t savedDay = loadFile(LASTWRITE_FILE).toInt();
                Serial.printlnf("Data last saved on day #%d", savedDay);
                Serial.printlnf("Today is #%d", dayNow);

                // Check day number, act accordingly.
                if (dayNow == savedDay + 1)
                {
                    Serial.println("Watt hours last written yesterday - updating arrays.");
                    loadArray(wh_yesterday, &wh_yesterday_Json, WH_TDAY_FILE, WH_TDAY_KEY);
                    writeFile(WH_TDAY_FILE, wh_yesterday_Json);
                    writeFile(WH_YSTDAY_FILE, WH_YSTDAY_INITSTRING);
                }
                else if (dayNow == savedDay)
                {
                    Serial.println("Saved watt hours are up to date - update arrays.");
                    loadArray(wh_today, &wh_today_Json, WH_TDAY_FILE, WH_TDAY_KEY);
                    loadArray(wh_yesterday, &wh_yesterday_Json, WH_YSTDAY_FILE, WH_YSTDAY_KEY);
                }
                else
                {
                    Serial.println("Saved watt hours out of date. Initializing arrays to 0");
                    writeFile(WH_TDAY_FILE, WH_TDAY_INITSTRING);
                    writeFile(WH_YSTDAY_FILE, WH_YSTDAY_INITSTRING);
                    loadArray(wh_today, &wh_today_Json, WH_TDAY_FILE, WH_TDAY_KEY);
                    loadArray(wh_yesterday, &wh_yesterday_Json, WH_YSTDAY_FILE, WH_YSTDAY_KEY);
                }
                return;
            }
            else
            {
                Serial.println("No valid time - maybe we dont have net?");
                return;
            }
        }
        else
        {
            Serial.println("Error initializing watt-Hour files.");
            return;
        }
    }
    else if (lastWriteExist < 0)
        Serial.println("Error initializing lastWrite.");
    else
    {
        // Files were created just now. Load initial data.
        loadArray(wh_today, &wh_today_Json, WH_TDAY_FILE, WH_TDAY_KEY);
        loadArray(wh_yesterday, &wh_yesterday_Json, WH_YSTDAY_FILE, WH_YSTDAY_KEY);
    }
}

/**
 * @brief Reads watt hour info from files, and writes it to the Jsons & int arrays.
 *
 * @param whArray
 * @param whJson
 * @param fileName
 * @param keyString
 */
void loadArray(int *wattHrArray, String *wattHrJson, String fileName, const char *keyString)
{
    *wattHrJson = loadFile(fileName);
    JSONVar wattHrObject = JSON.parse(*wattHrJson);
    JSONVar wattHrArrayObject = wattHrObject[keyString];
    for (int i = 0; i < 24; i++)
        wattHrArray[i] = wattHrArrayObject[i];
}

/**
 * @brief Updates the files with current info.
 *
 */
void updateFiles()
{
    Serial.println("Updating files in storage.");
    writeFile(WH_TDAY_FILE, wh_today_Json);
    writeFile(WH_YSTDAY_FILE, wh_yesterday_Json);
    writeFile(LASTWRITE_FILE, Time.format(Time.now(), "%j"));
}