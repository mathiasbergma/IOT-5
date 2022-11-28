#include "application.h"
#include "../lib/HttpClient/src/HttpClient.h"
/**
* Declaring the variables.
*/
unsigned int nextTime = 0;    // Next time to contact the server
HttpClient http;

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
  { "Content-Length", "0" },
  { "Accept" , "application/json" },
//{ "Accept" , "*/*"},
{ NULL, NULL } // NOTE: Always terminate headers will NULL
};

http_request_t request;
http_response_t response;


// to get a verbose output from these actions
#define LOGGING  

void htttttp() {

Serial.println();
Serial.println("Application>\tStart of Loop.");
// Request path and body can be set at runtime or at setup.
request.hostname = "api.energidataservice.dk";
request.port = 80;
request.path = "/dataset/Elspotprices?filter=%7B%22PriceArea%22%3A%22DK2%22%7D&start=2022-11-27T00%3A00&offset=0&limit=24&columns=SpotPriceDKK&sort=HourDK%20ASC&timezone=dk";

// The library also supports sending a body with your request:
//request.body = "{\"key\":\"value\"}";

// Get request
http.get(request, response, headers);
Serial.print("Application>\tResponse status: ");
Serial.println(response.status);

Serial.print("Application>\tHTTP Response Body: ");
Serial.println(response.body);

return;
}