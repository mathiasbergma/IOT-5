
#include "particle.h"
const char* argonName = "ArgonPM";
const char* serviceUuid_c     = "3f1a1596-ee7f-42bd-84d1-b1a294f82ecf";
const char* watt_c           = "b4250401-fb4b-4746-b2b0-93f0e61122c6";
const char* DkkYesterday_c    = "b4250406-fb4b-4746-b2b0-93f0e61122c6";
const char* DkkToday_c        = "b4250402-fb4b-4746-b2b0-93f0e61122c6"; 
const char* DkkTomorrow_c     = "b4250403-fb4b-4746-b2b0-93f0e61122c6";
const char* WhrToday_c        = "b4250404-fb4b-4746-b2b0-93f0e61122c6";
const char* WhrYesterday_c    = "b4250405-fb4b-4746-b2b0-93f0e61122c6";

const BleUuid powermonitorserviceUuid(serviceUuid_c);

const BleUuid wattcharacteristicUuid(watt_c);
const BleUuid DkkyesterdaycharacteristicUuid(DkkYesterday_c);
const BleUuid DkktodaycharacteristicUuid(DkkToday_c);
const BleUuid DkktomorrowcharacteristicUuid(DkkTomorrow_c);
const BleUuid WhrTodaycharacteristicUuid (WhrToday_c);
const BleUuid WhrYesterdayCharacteristicUuid(WhrYesterday_c);


BleCharacteristic WattCharacteristic("Watt now", BleCharacteristicProperty::NOTIFY, wattcharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic DkkYesterdayCharacteristic("DKK Yesterday", BleCharacteristicProperty::NOTIFY, DkkyesterdaycharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic DkkTodayCharacteristic("DKK Today", BleCharacteristicProperty::NOTIFY, DkktodaycharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic DkkTomorrowCharacteristic("DKK Tomorrow", BleCharacteristicProperty::NOTIFY, DkktomorrowcharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic WhrTodayCharacteristic("Whr Today", BleCharacteristicProperty::NOTIFY, WhrTodaycharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic WhrYesterdayCharacteristic("Whr Yesterday", BleCharacteristicProperty::NOTIFY, WhrYesterdayCharacteristicUuid, powermonitorserviceUuid);


//bool device_connected = false; //not in use yet
//bool advertising_BLE = false;  //not in use yet
/*
    //To send values do the following in main loop
    if (BLE.connected()) {
            WattCharacteristic.setValue(&watt,1);   // to send int value
            DkkTodayCharacteristic.setValue("{\"pricestoday\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}");  // string mKr/kwhr
            DkkTomorrowCharacteristic.setValue("{\"pricestomorrow\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}"); // string mKr/kwhr
            WhrTodayCharacteristic.setValue("{\"WHr_today\":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]}"); // Whr used in the corrisponding hour
        }
*/
void BLEOnConnectcallback(const BlePeerDevice& peer, void* context);


void ble_setup(void){
    //BLE.selectAntenna(BleAntennaType::EXTERNAL);// change to internal if an external antenna is not attached.
    BLE.on();
    //*****BLE setup*****
    BLE.addCharacteristic(WattCharacteristic);
    BLE.addCharacteristic(DkkYesterdayCharacteristic);
    BLE.addCharacteristic(DkkTodayCharacteristic);
    BLE.addCharacteristic(DkkTomorrowCharacteristic);
    BLE.addCharacteristic(WhrTodayCharacteristic);
    BLE.addCharacteristic(WhrYesterdayCharacteristic);
    BleAdvertisingData advData;
    advData.appendLocalName(argonName);
    advData.appendServiceUUID(powermonitorserviceUuid);
    BLE.advertise(&advData);
    Serial.println("Waiting for BLEclient connection...");
    BLE.onConnected(BLEOnConnectcallback);

}

