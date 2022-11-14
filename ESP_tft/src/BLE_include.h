
#include <Arduino.h>
#include "BLEDevice.h"
#define BLE_server "ArgonPM"
#include <string>
#include <stdlib.h>
#include <Arduino_JSON.h>
#define CONFIG_BT_ENABLED
volatile int power;
volatile double pricetoday[24];
volatile double pricesyesterday[24];
volatile double pricetomorrow[24];
volatile double WHrToday_arr[24];
volatile double WHryesterday_arr[24];

/* Specify the Service UUID of Server  and characteristics*/

const char* serviceUuid_c     = "3f1a1596-ee7f-42bd-84d1-b1a294f82ecf";
const char* watt_c            = "b4250401-fb4b-4746-b2b0-93f0e61122c6";
const char* DkkYesterday_c    = "b4250406-fb4b-4746-b2b0-93f0e61122c6";
const char* DkkToday_c        = "b4250402-fb4b-4746-b2b0-93f0e61122c6"; 
const char* DkkTomorrow_c     = "b4250403-fb4b-4746-b2b0-93f0e61122c6";
const char* WhrToday_c        = "b4250404-fb4b-4746-b2b0-93f0e61122c6";
const char* WhrYesterday_c    = "b4250405-fb4b-4746-b2b0-93f0e61122c6";

static BLEUUID serviceUUID(serviceUuid_c);

static BLEUUID wattcharacteristicUuid(watt_c);
static BLEUUID DkkyesterdaycharacteristicUuid(DkkYesterday_c);
static BLEUUID DkktodaycharacteristicUuid(DkkToday_c);
static BLEUUID DkktomorrowcharacteristicUuid(DkkTomorrow_c);
static BLEUUID WhrTodaycharacteristicUuid(WhrToday_c);
static BLEUUID WhrYesterdayCharacteristicUuid(WhrYesterday_c);

static void wattNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void DkkYesterdayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void DkkTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void DkkTomorrowNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void WhrTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void WhrYesterdayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

static BLEUUID DescripterUUID("00002901-0000-1000-8000-00805f9b34fb");

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic *wattcharacteristic;
static BLERemoteCharacteristic *Dkkyesterdaycharacteristic;
static BLERemoteCharacteristic *Dkktodaycharacteristic;
static BLERemoteCharacteristic *Dkktomorrowcharacteristic;
static BLERemoteCharacteristic *WhrTodaycharacteristic;
static BLERemoteCharacteristic *Whryesterdaycharacteristic;

const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

BLEClient *pClient;

bool connectToServer(BLEAddress pAddress)
{
    Serial.println("enter connect to server");
    vTaskDelay(50/portTICK_PERIOD_MS);
    pClient = BLEDevice::createClient();

    if(!pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM)){
        Serial.println("failed connection");
        return (false);
    }
    Serial.println(" - Connected successfully to server");

    BLERemoteService *pRemoteService = pClient->getService(serviceUuid_c);

    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        return (false);
    }
    Serial.println(" got to here!");
    wattcharacteristic = pRemoteService->getCharacteristic(wattcharacteristicUuid);
    Dkkyesterdaycharacteristic = pRemoteService->getCharacteristic(DkkyesterdaycharacteristicUuid);
    Dkktodaycharacteristic = pRemoteService->getCharacteristic(DkktodaycharacteristicUuid);
    Dkktomorrowcharacteristic = pRemoteService->getCharacteristic(DkktomorrowcharacteristicUuid);
    WhrTodaycharacteristic = pRemoteService->getCharacteristic(WhrTodaycharacteristicUuid);
    Whryesterdaycharacteristic = pRemoteService->getCharacteristic(WhrYesterdayCharacteristicUuid);

    if (wattcharacteristic == nullptr || Dkktodaycharacteristic == nullptr)
    {
        Serial.print("Failed to find our characteristic UUID");
        return false;
    }
    Serial.println(" Characteristics Found!");
    delay(10);

    wattcharacteristic->registerForNotify(wattNotifyCallback);
    Dkkyesterdaycharacteristic->registerForNotify(DkkYesterdayNotifyCallback);
    Dkktodaycharacteristic->registerForNotify(DkkTodayNotifyCallback);
    Dkktomorrowcharacteristic->registerForNotify(DkkTomorrowNotifyCallback);
    WhrTodaycharacteristic->registerForNotify(WhrTodayNotifyCallback);
    Whryesterdaycharacteristic->registerForNotify(WhrYesterdayNotifyCallback);
    Serial.println(" got all the way here!");
    return true;
}

void connect_argonpm(void)
{
    if (connectToServer(*pServerAddress))
    {
        Serial.println("We are now connected to the BLE Server.");
        wattcharacteristic->getDescriptor(DescripterUUID)->writeValue((uint8_t *)notificationOn, 2, true);
        Dkkyesterdaycharacteristic->getDescriptor(DescripterUUID)->writeValue((uint8_t *)notificationOn,2,true);
        Dkktodaycharacteristic->getDescriptor(DescripterUUID)->writeValue((uint8_t *)notificationOn, 2, true);
        Dkktomorrowcharacteristic->getDescriptor(DescripterUUID)->writeValue((uint8_t *)notificationOn, 2, true);
        WhrTodaycharacteristic->getDescriptor(DescripterUUID)->writeValue((uint8_t *)notificationOn, 2, true);
        Whryesterdaycharacteristic->getDescriptor(DescripterUUID)->writeValue((uint8_t *)notificationOn, 2, true);

    }
    else
    {
        Serial.println("Failed to connect to server!");
       
    }
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.getName() == BLE_server)
        {
            advertisedDevice.getScan()->stop();
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());

            Serial.println("Device found. Connecting!");
        }
    }
};

static void wattNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{

    char buf[300] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
        buf[i + 1] = '\0';
    }
    Serial.print(buf);
    Serial.print('\n');

    JSONVar myObject = JSON.parse(buf);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing Watt input failed!");
        return;
    }
    if (myObject.hasOwnProperty("watt"))
    {
        // Serial.print("\nmyObject[\"watt\"] = ");

        // Serial.println((int)myObject["watt"]);

        power = (int)myObject["watt"];
    }
}

static void DkkTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    char buf[300] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
        buf[i + 1] = '\0';
    }
    Serial.print(buf);
    Serial.print('\n');

    JSONVar myObject = JSON.parse(buf);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing pricestoday input failed!");
        return;
    }
    if (myObject.hasOwnProperty("pricestoday"))
    {
        JSONVar myArray = myObject["pricestoday"];
        for (size_t d = 0; d < 24; d++)
        {
            pricetoday[d] = myArray[d];
        }
    }
}

static void DkkYesterdayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{

    char buf[300] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
        buf[i + 1] = '\0';
    }
    Serial.print(buf);
    Serial.print('\n');

    JSONVar myObject = JSON.parse(buf);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing pricesyesterday input failed!");
        return;
    }
    if (myObject.hasOwnProperty("pricesyesterday"))
    {
        JSONVar myArray = myObject["pricesyesterday"];
        for (size_t d = 0; d < 24; d++)
        {
            pricesyesterday[d] = myArray[d];
        }
    }
}

static void DkkTomorrowNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{

    char buf[300] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
        buf[i + 1] = '\0';
    }
    Serial.print(buf);
    Serial.print('\n');

    JSONVar myObject = JSON.parse(buf);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing pricestomorrow input failed!");
        return;
    }
    if (myObject.hasOwnProperty("pricestomorrow"))
    {
        JSONVar myArray = myObject["pricestomorrow"];
        for (size_t d = 0; d < 24; d++)
        {
            pricetomorrow[d] = myArray[d];
        }
    }
}
static void WhrTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    char buf[300] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
    }
    buf[length] = '\0';

    Serial.print(buf);
    Serial.print('\n');

    JSONVar myObject = JSON.parse(buf);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing WHr_today input failed!");
        return;
    }
    if (myObject.hasOwnProperty("WHr_today"))
    {
        JSONVar myArray = myObject["WHr_today"];
        for (size_t d = 0; d < 24; d++)
        {
            WHrToday_arr[d] = myArray[d];
        }
    }
}

static void WhrYesterdayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    char buf[300] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
    }
    buf[length] = '\0';

    Serial.print(buf);
    Serial.print('\n');

    JSONVar myObject = JSON.parse(buf);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing WHr_yesterday input failed!");
        return;
    }
    if (myObject.hasOwnProperty("WHr_yesterday"))
    {
        JSONVar myArray = myObject["WHr_yesterday"];
        for (size_t d = 0; d < 24; d++)
        {
            WHryesterday_arr[d] = myArray[d];
        }
    }
}
