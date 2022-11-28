#include "application.h"
#include "../lib/HttpClient/src/HttpClient.h"
#include "../lib/Json/src/Arduino_JSON.h"
#include "state_variables.h"

extern double * cost_tomorrow;
extern const struct transport_t transport;
/**
 * Declaring the variables.
 */
unsigned int nextTime = 0; // Next time to contact the server
HttpClient http;

// Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {
    {"Content-Length", "0"},
    {"Accept", "application/json"},
    //{ "Accept" , "*/*"},
    {NULL, NULL} // NOTE: Always terminate headers will NULL
};

http_request_t request;
http_response_t response;

// to get a verbose output from these actions
#define LOGGING

bool get_data_http(int day)
{

    String path_s = "/dataset/Elspotprices?filter=%7B%22PriceArea%22%3A%22DK2%22%7D&start=";
    path_s += String::format("%d-%02d-%02d", Time.year(), Time.month(), day);
    path_s += "T00%3A00&offset=0&limit=24&columns=SpotPriceDKK&sort=HourDK%20ASC&timezone=dk";
    // Request path and body can be set at runtime or at setup.
    request.hostname = "api.energidataservice.dk";
    request.port = 80;
    request.path = path_s.c_str();

    // The library also supports sending a body with your request:
    // request.body = "{\"key\":\"value\"}";

    // Get request
    http.get(request, response, headers);
    // Serial.print("Application>\tResponse status: ");
    // Serial.println(response.status);
    if (response.status == 200)
    {
        Serial.println("success response code 200");
    }
    else
    {
        Serial.println("bad response");
        return false;
    }

    Serial.println(response.body);

    JSONVar myObject = JSON.parse(response.body);
    if (JSON.typeof(myObject) == "undefined")
    {
        Serial.println("Parsing input failed!");
        return false;
    }
    if (myObject.hasOwnProperty("records"))
    {
        JSONVar myArray = myObject["records"];
        Serial.println(myArray[0]);

        for (int i = 0; i < 24; i++)
        {
            JSONVar myHour = myArray[i];

            if (JSON.typeof(myHour["SpotPriceDKK"]) == "undefined")
            {
                Serial.println("parsing failed 2");
                return false;
            }
            else
            {
                cost_tomorrow[i] = (double)myHour["SpotPriceDKK"]/1000.0;
                if (i >= 0 && i < 7)
                {
                    cost_tomorrow[i] += transport.low;
                }
                else if (i > 16 && i < 22)
                {
                    cost_tomorrow[i] += transport.high;
                }
                else
                {
                    cost_tomorrow[i] += transport.medium;
                }
            }
        }
    }
    CALCULATE = true;
    return true;
}