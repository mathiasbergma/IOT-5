
#include <Arduino.h>
#include "BLEDevice.h"
#define BLE_server "ArgonPM"
#include <string>
#include <stdlib.h>

/* Specify the Service UUID of Server  and characteristics*/

const char *serviceUuid = "3f1a1596-ee7f-42bd-84d1-b1a294f82ecf";
const char *watt = "b4250401-fb4b-4746-b2b0-93f0e61122c6";
const char *DkkToday = "b4250402-fb4b-4746-b2b0-93f0e61122c6";
const char *DkkTomorrow = "b4250403-fb4b-4746-b2b0-93f0e61122c6";
const char *WhrToday = "b4250404-fb4b-4746-b2b0-93f0e61122c6";

static BLEUUID serviceUUID(serviceUuid);

static BLEUUID wattcharacteristicUuid(watt);
static BLEUUID DkktodaycharacteristicUuid(DkkToday);
static BLEUUID DkktomorrowcharacteristicUuid(DkkTomorrow);
static BLEUUID WhrTodaycharacteristicUuid(WhrToday);

static void wattNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void DkkTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void DkkTomorrowNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);
static void WhrTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify);

static BLEUUID DescripterUUID("00002901-0000-1000-8000-00805f9b34fb");

static boolean doConnect = false;
static boolean connected = false;

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic *wattcharacteristic;
static BLERemoteCharacteristic *Dkktodaycharacteristic;
static BLERemoteCharacteristic *Dkktomorrowcharacteristic;
static BLERemoteCharacteristic *WhrTodaycharacteristic;

const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};

bool connectToServer(BLEAddress pAddress)
{
    BLEClient *pClient = BLEDevice::createClient();

    pClient->connect(pAddress, BLE_ADDR_TYPE_RANDOM);
    Serial.println(" - Connected successfully to server");

    BLERemoteService *pRemoteService = pClient->getService(serviceUuid);

    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        return (false);
    }

    wattcharacteristic = pRemoteService->getCharacteristic(wattcharacteristicUuid);
    Dkktodaycharacteristic = pRemoteService->getCharacteristic(DkktodaycharacteristicUuid);
    Dkktomorrowcharacteristic = pRemoteService->getCharacteristic(DkktomorrowcharacteristicUuid);
    WhrTodaycharacteristic = pRemoteService->getCharacteristic(WhrTodaycharacteristicUuid);

    if (wattcharacteristic == nullptr || Dkktodaycharacteristic == nullptr)
    {
        Serial.print("Failed to find our characteristic UUID");
        return false;
    }
    Serial.println(" Characteristics Found!");

    wattcharacteristic->registerForNotify(wattNotifyCallback);
    Dkktodaycharacteristic->registerForNotify(DkkTodayNotifyCallback);
    Dkktomorrowcharacteristic->registerForNotify(DkkTomorrowNotifyCallback);
    WhrTodaycharacteristic->registerForNotify(WhrTodayNotifyCallback);
    return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.getName() == BLE_server)
        {
            advertisedDevice.getScan()->stop();
            pServerAddress = new BLEAddress(advertisedDevice.getAddress());

            doConnect = true;
            Serial.println("Device found. Connecting!");
        }
    }
};

static void wattNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{

    char buf[255] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
    }
    buf[length]= '\0';

    Serial.print("\nWatts: ");
    Serial.print(buf);
    Serial.print('\n');
}

static void DkkTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
       char buf[255] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
        buf[i + 1] = '\0';
    }
    Serial.print("\nDKK today: ");
    
        Serial.print(buf+ '\n');

}

static void DkkTomorrowNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{

       char buf[255] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
        buf[i + 1] = '\0';
    }
    Serial.print("\nDKK tomorrow: ");
    
        Serial.print(buf+ '\n');
}
static void WhrTodayNotifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    char buf[255] = "";
    for (size_t i = 0; i < length; i++)
    {
        buf[i] = pData[i];
    }
    buf[length]= '\0';

    Serial.print("\nWattHr: ");
    Serial.print(buf);
    Serial.print('\n');
}
