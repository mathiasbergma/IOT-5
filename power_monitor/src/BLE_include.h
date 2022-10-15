
#include "particle.h"
const char* argonName = "ArgonPM";
const char* serviceUuid     = "3f1a1596-ee7f-42bd-84d1-b1a294f82ecf";
const char* watt            = "b4250401-fb4b-4746-b2b0-93f0e61122c6"; 
const char* DkkToday        = "b4250402-fb4b-4746-b2b0-93f0e61122c6"; 
const char* DkkTomorrow     = "b4250403-fb4b-4746-b2b0-93f0e61122c6";
const char* WhrToday        = "b4250404-fb4b-4746-b2b0-93f0e61122c6";

const BleUuid powermonitorserviceUuid(serviceUuid);

const BleUuid wattcharacteristicUuid(watt);
const BleUuid DkktodaycharacteristicUuid(DkkToday);
const BleUuid DkktomorrowcharacteristicUuid(DkkTomorrow);
const BleUuid WhrTodaycharacteristicUuid (WhrToday);


BleCharacteristic WattCharacteristic("Watt now", BleCharacteristicProperty::NOTIFY, wattcharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic DkkTodayCharacteristic("DKK Today", BleCharacteristicProperty::NOTIFY, DkktodaycharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic DkkTomorrowCharacteristic("DKK Today", BleCharacteristicProperty::NOTIFY, DkktomorrowcharacteristicUuid, powermonitorserviceUuid);
BleCharacteristic WhrTodayCharacteristic("DKK Today", BleCharacteristicProperty::NOTIFY, WhrTodaycharacteristicUuid, powermonitorserviceUuid);


//bool device_connected = false; //not in use yet
//bool advertising_BLE = false;  //not in use yet
/*
    To send values do the following after setup
    if (BLE.connected()) {
            WattCharacteristic.setValue(&watt,1);   // to send int value
            DkkTodayCharacteristic.setValue("{pricestoday:[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]");  // string mKr/kwhr
            DkkTomorrowCharacteristic.setValue("{pricestomorrow:[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]"); // string mKr/kwhr
            WhrTodayCharacteristic.setValue("{WHr_today:[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24]"); // Whr used in the corrisponding hour
        }


*/


void ble_setup(void){
    BLE.selectAntenna(BleAntennaType::EXTERNAL);// change to internal if an external antenna is not attached.
    BLE.on();
    //*****BLE setup*****
    BLE.addCharacteristic(WattCharacteristic);
    BLE.addCharacteristic(DkkTodayCharacteristic);
    BLE.addCharacteristic(DkkTomorrowCharacteristic);
    BLE.addCharacteristic(WhrTodayCharacteristic);
    BleAdvertisingData advData;
    advData.appendLocalName(argonName);
    advData.appendServiceUUID(powermonitorserviceUuid);
    BLE.advertise(&advData);
    Serial.println("Waiting for BLEclient connection...");
}

