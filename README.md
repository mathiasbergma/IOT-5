# IOT-5
## 5. Semester IOT projekt using Particle Argon

### Project requirements
  - The device should be able to access energidataservice to ascertain the electricity prices for the day  
  - The hours of the day where prices are below a predetermined relative threshold should be made available to the user  
  - A near realtime measurement of total power consumption, at the installed site, should be made available to the user   

### Hardware   
***Processor***   
  - Particle Argon   
  - Screen   
   
***Sensors***   
  - LDR or light Sensitive Diode
  
  ![alt text](https://github.com/mathiasbergma/Power_monitor/blob/master/Schematic_Light_Sensor.jpg
   
***Supply***   
  - 5V Power supply   

### Further functionality   
- Screen to display current consumption og low price hours

#### Use the following link for Particle webhook API call
https://api.energidataservice.dk/dataset/Elspotprices?offset=0&start={{{year}}}-{{{month}}}-{{{day}}}T{{{hour}}}:00&end={{{year}}}-{{{month}}}-{{{day_two}}}T00:00&filter=%7B%22PriceArea%22:%22DK2%22%7D&sort=HourUTC%20ASC&timezone=dk

This project can work with Particle webhooks or direct HTTP requests to api.energidataservice.dk. When using direct HTTP requests, no external setup is required with regards to getting electricity prices

#### Response template 
In Particle Webhook setup, use the following response template to extract the data needed:
***{{#records}}!{{HourDK}},{{SpotPriceDKK}}{{/records}}***


#### Deployment diagram
![alt text](https://github.com/mathiasbergma/Power_monitor/blob/master/UML_Deployment4.jpg)


This project is intented to work with a Particle Argon (Responsible for getting electricity prices and monitoring power consumption) as well as an ESP32 with a tft-display.
If an ESP32 is not awailable, the Argon code can be modified to use MQTT instead. This is done by uncommenting USEMQTT
´´´ // #define USEMQTT ´´´
