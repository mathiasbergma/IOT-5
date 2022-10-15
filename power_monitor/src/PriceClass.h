#pragma once

#include "application.h"
#include <map>
#include <string>
#include <vector>

#define LOW_THRESHOLD_FACTOR 0.3

class PriceClass
{

  private:
    struct HourPrice
    {
        uint8_t hour;
        float price;
    };

    std::map<uint8_t, std::string> messageParts;
    bool newPricesReceived;
    bool messageDataReady;
    std::vector<HourPrice> currentHourPrices;
    std::vector<HourPrice> lowPriceHours;
    HourPrice currentHighest;
    HourPrice currentLowest;
    std::string lastUpdate;

    Timer *updateTimer;

    void getpricesSubscriptionHandler(const char *getpricesEvent, const char *data);
    void pricesSubscriptionHandler(const char *pricesEvent, const char *data);
    void requestPriceUpdate(int day);
    void timedUpdate();
    void timedCountDown(uint period);

  public:
    PriceClass();
    void assembleMessageData();                // Assemble the data to something meaningful.
    void updatePrices();                       // publish to "elpriser to fire the webhook"
    std::string getLowPriceIntervals();        // Returns prices from the local attributes.
    void initSubscriptions();                  // Setup the particle event subscriptions for the webhook.
    std::string getLastUpdate();               // Returns a time-string for the last price update (ex: "Wed May 21 01:08:47 2014")
    std::vector<HourPrice> getlowPriceHours(); // Returns a vector of the current calculated low price hours.
    bool pricesUpdated();                      // Check if there's new prices to be read.
    bool isMessageDataReady();                 // Check if message data has arrived, and needs to be man-handled.
};