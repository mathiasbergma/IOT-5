/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "c:/Users/mikeh/vscode-particle/Power_monitor/power_monitor/src/ElecPrice.ino"
#include "../lib/MQTT/src/MQTT.h"
#include "PriceClass.h"
#include "Sensor.h"
#include "application.h"
#include <string>
#include "BLE_include.h"
#include "../lib/ntp-time/src/ntp-time.h"
#include "../lib/Json/src/Arduino_JSON.h"

//#define USE_MQTT

void setup();
void loop();
void publishPrices(String prices);
void publishPower(int currentPower);
#line 12 "c:/Users/mikeh/vscode-particle/Power_monitor/power_monitor/src/ElecPrice.ino"
#ifdef USE_MQTT
#define MQTT_HOST "192.168.1.102"
#define PORT 1883
#endif

// Prototypes
void mqttCallback(char *topic, byte *payload, unsigned int length);
void mqttKeepAlive();
void publishPrices(String);
void publishPower(int);

void pricetoday_SubscriptionHandler(const char *event, const char *data);
void ask_for_todays_prices(void);
String pricestoday_Json;
volatile double pricetoday_arr[24];
bool NewpricesToday = false;

// Declare objects
PriceClass prices;
Sensor wattSensor;
// MQTT mqttClient(MQTT_HOST, PORT, 512, 30, mqttCallback);

char *ntp_denmark = "dk.pool.ntp.org"; 
NtpTime* ntpTime;

SYSTEM_THREAD(ENABLED);

// ###################### SETUP #################################
void setup()
{
    waitUntil(Particle.connected);
    ntpTime = new NtpTime(15,ntp_denmark);  // Do an ntp update every 15 minutes;
    ntpTime->start();
    Time.zone(1);
    Time.beginDST();

    // Set up pin reading.
    wattSensor.initSensor();

    // setup BLE
    ble_setup();

    // Set up MQTT
#ifdef USE_MQTT
    Serial.printf("Return value: %d\n", mqttClient.connect("sparkclient_" + String(Time.now()), "mqtt", "mqtt"));
    if (mqttClient.isConnected())
    {
        mqttClient.publish("power/get", "Argon Booting.");
    }
#endif

    // Initiate particle subscriptions & request first price update.
    prices.initSubscriptions();
    Particle.subscribe("pricestoday_ask", pricetoday_SubscriptionHandler, MY_DEVICES);

    ask_for_todays_prices();
}

// ##################### MAIN LOOP ##############################
void loop()
{
    // Is message ready to be man-handled?
    if (prices.isMessageDataReady())
        prices.assembleMessageData();

    // Should we publish on mqtt/thingspeak/display?
    if (prices.pricesUpdated())
    {
        String priceString = prices.getLowPriceIntervals().c_str();
    }

    if (wattSensor.checkForNewReading())
    {
        // publishPower(wattSensor.getCurrentReading());
        if (BLE.connected())
        {
            char buffer[255];
            sprintf(buffer, "{\"watt\":%d}", wattSensor.getCurrentReading());
            WattCharacteristic.setValue(buffer);
            DkkTodayCharacteristic.setValue(pricestoday_Json);
            //DkkTodayCharacteristic.setValue("{\"pricestoday\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}");
            DkkTomorrowCharacteristic.setValue("{\"pricestomorrow\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}");
            WhrTodayCharacteristic.setValue("{\"WHr_today\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}");
        }
    }

    if(NewpricesToday){
        DkkTodayCharacteristic.setValue(pricestoday_Json);
        NewpricesToday = false;

    }

#ifdef USE_MQTT
    mqttKeepAlive();
#endif

    // Waitasecond...
    delay(1000);
}

// ############## FUNCTION IMPLEMENTATIONS #################################

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    // The MQTT client will call this function, when receiving a message on a subscribed topic.
    // This provides a way to send instructions - for example to trigger a price update.
}
/*
void mqttKeepAlive()
{
    if (mqttClient.isConnected())
    {
        mqttClient.loop();
    }
    else
    {
        Serial.printf("Client disconnected - trying to reconnect:\n");
        mqttClient.connect("sparkclient_" + String(Time.now()), "mqtt", "mqtt");
    }
}
*/

void publishPrices(String prices)
{
    Particle.publish("Low price hours", prices);

#ifdef USE_MQTT
    if (mqttClient.isConnected())
    {
        mqttClient.publish("prices", prices);
    }

#endif
}

void publishPower(int currentPower)
{
    String powerString = String::format("%d", currentPower);
    Particle.publish("Power", powerString);

#ifdef USE_MQTT
    if (mqttClient.isConnected())
    {
        mqttClient.publish("power", powerString);
    }

#endif
}

void ask_for_todays_prices(void){
String data = String::format("{ \"year\": \"%d\", ", Time.year()) +
                  String::format("\"month\": \"%02d\", ", Time.month()) +
                  String::format("\"day\": \"%02d\", ", Time.day()) +
                  String::format("\"hour\": \"%02d\" }", Time.hour());

    // Trigger the integration
    Particle.publish("pricestoday", data, PRIVATE);
}

void pricetoday_SubscriptionHandler(const char *event, const char *data){
    //Serial.println(data);
    //Serial.printf("\n");

    String jsonbuff = String::format("{\"pricestoday\":[") +
                    String::format(data) +
                    String::format("0]}");
    JSONVar myObject = JSON.parse(jsonbuff);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing todays prices input failed!");
        return;
    }
    if (myObject.hasOwnProperty("pricestoday"))
    {
        pricestoday_Json = String::format("{\"pricestoday\":[");
        JSONVar myArray = myObject["pricestoday"];
        int maxbuf =0;
        for (size_t h = 0; h < 24; h++)
        {
            pricestoday_Json += String::format("%d,",(int)myArray[h]);
            pricetoday_arr[h] = myArray[h];
            if (maxbuf<(int)myArray[h]){
                maxbuf=(int)myArray[h];
            }
        }
        pricestoday_Json += String::format("%d]}",maxbuf);
        Serial.println(pricestoday_Json);
        
    }
    

    NewpricesToday=true;
    //Serial.println(pricestoday);

}