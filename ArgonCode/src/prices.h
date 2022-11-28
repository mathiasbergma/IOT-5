
#include "../lib/Json/src/Arduino_JSON.h"

#include "application.h"

TCPClient client;

bool Httprequest_today(void)
{
  String Json_message;
  client.connect("api.energidataservice.dk", 80);
  if (client.connected())
  {
    Serial.println("connected");
    client.println("GET /dataset/Elspotprices?filter=%7B%22PriceArea%22%3A%22DK2%22%7D&start=2022-11-27T00%3A00&offset=0&limit=24&columns=SpotPriceDKK&sort=HourDK%20ASC&timezone=dk HTTP/1.0");
    client.println("Host: api.energidataservice.dk");
    client.println("Content-Length: 0");
    client.println("Content-Type: application/json");
    client.println();
    // 
  }
  delay(2000);
  bool print_next = false;
  while (client.connected())
  {
    uint32_t bytesAvail = client.available();
    uint32_t ptr = 0;
    uint8_t buffer[500];
    client.read(buffer, bytesAvail);
    char message[bytesAvail+1]= "\0";

    for (int i=0;i<bytesAvail;i++){
      message[i] = (char)buffer[i];
    }
    
      Serial.print(message);

  }
  // if temp is char string, term it here temp[ptr] = '\0';
  

  if (!client.connected())
  {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }

  return true;
}
