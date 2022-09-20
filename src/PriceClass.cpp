#include "PriceClass.h"

#include <string>

using namespace std;

// Constructor
PriceClass::PriceClass()
    : last_read{millis()},
      range{INITIAL_RANGE} {}

void PriceClass::update_power() {}

void PriceClass::request_price_data(int day) // was: get_data
{
    // Reset counters
    transmission_count = 0; // rec_cnt
    range = INITIAL_RANGE;
    cnt = 0;
    temp[0] = 0;

    // Set up the data string
    String data = "{ \"year\": \"" + (String)Time.year() + "\", " +
                  String::format("\"month\": \"%02d\", ", Time.month()) +
                  String::format("\"day\": \"%02d\", ", day) +
                  String::format("\"day_two\": \"%02d\", ", (day + 2)) +
                  String::format("\"hour\": \"%02d\" }", Time.hour());

    // Publish data string, to trigger the webhook.
    if (Particle.connected())
    {
        if (!(Particle.publish(EVENT_NAME, data)))
            Serial.println("ERROR PUBLISHING");
    }
    else
        Serial.println("Particle Not Connected.");
}

void PriceClass::price_subscription_handler(const char *event_name, const char *data) {}