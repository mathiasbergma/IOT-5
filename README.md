# IOT-5
## 5. Semester IOT projekt using Particle Argon

### Project requirements
  1. The device should be able to access energidataservice.dk, to ascertain the electricity prices for the day.
  2. A near realtime measurement of total power consumption, at the installed site, should be made available to the user.
  3. Current consumption in watt, historic consumption (kWHr) and accumulated price for the current day, should be presented on a display.
  4. Hourly prices for the next day should be fetched and forwarded to the display module.
  5. Transport prices must be taken into account for the given area provider, in the form of variables that can added to the cost

### Hardware   
***Processor***   
  - Particle Argon   
  - Screen   
   
***Sensors***   
  - LDR or light Sensitive Diode
  
![alt text](https://github.com/mathiasbergma/Power_monitor/blob/master/Schematic_Light_sensor.jpg)
   
***Supply***   
  - 5V Power supply   

### Further functionality   
- Screen to display current consumption og low price hours

#### Use the following link for Particle webhook API call
```
https://api.energidataservice.dk/dataset/Elspotprices?offset=0&start={{{year}}}-{{{month}}}-{{{day}}}T{{{hour}}}:00&end={{{year}}}-{{{month}}}-{{{day_two}}}T00:00&filter=%7B%22PriceArea%22:%22DK2%22%7D&sort=HourUTC%20ASC&timezone=dk
```

This project can work with Particle webhooks or direct HTTP requests to api.energidataservice.dk. When using direct HTTP requests, no external setup is required with regards to getting electricity prices

#### Response template 
In Particle Webhook setup, use the following response template to extract the data needed:
```
***{{#records}}!{{HourDK}},{{SpotPriceDKK}}{{/records}}***
```

#### Deployment diagram
![alt text](https://github.com/mathiasbergma/Power_monitor/blob/master/UML_Deployment4.jpg)


This project is intented to work with a Particle Argon (Responsible for getting electricity prices and monitoring power consumption) as well as an ESP32 with a tft-display.

If an ESP32 is not available, the Argon code can be modified to use MQTT instead. This is done by uncommenting USEMQTT in ElecPrice.ino

``` C 
// #define USEMQTT 
```
The user will also need to change

``` C
#define HOST "homeassistant.local"
#define PORT 1883
#define MQTT_USERNAME "mqtt"
#define MQTT_PASSWORD "mqtt"
```
in ElecPrice.ino to fit whatever MQTT-Broker the user would wish to utilize
