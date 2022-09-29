# IOT-5
## 5. Semester IOT projekt using Particle Argon

### Project requirements
***- The device should be able to access energidataservice to ascertain the electricity prices for the day***  
***- The hours of the day where prices are below a predetermined relative threshold should be made available to the user***  
***- A near realtime measurement of total power consumption, at the installed site, should be made available to the user***   

### Hardware   
***- Particle Argon***   
***- LDR or light Sensitive Diode***   
***- 5V Power supply***   

### Further development   
***- Screen to display current consumption og low price hours***

#### Use the following link for Particle webhook API call
https://api.energidataservice.dk/dataset/Elspotprices?offset=0&start={{{year}}}-{{{month}}}-{{{day}}}T{{{hour}}}:00&end={{{year}}}-{{{month}}}-{{{day_two}}}T00:00&filter=%7B%22PriceArea%22:%22DK2%22%7D&sort=HourUTC%20ASC&timezone=dk

#### Response template 
In Particle Webhook setup, use the following response template to extract the data needed:
***{{#records}}!{{HourDK}},{{SpotPriceDKK}}{{/records}}***
