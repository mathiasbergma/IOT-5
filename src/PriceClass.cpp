#include "PriceClass.h"
#include "application.h"
#include <string>

#define DATE_INDEX 8
#define HOUR_INDEX 11
#define PRICE_INDEX 20
#define MILLISEC_HOUR 3600000
#define MILLISEC_MINUTE 60000

// Constructor
PriceClass::PriceClass()
    : newPricesReceived{false},
      messageDataReady{false}
{
    updateTimer = new Timer(1000, &PriceClass::timedUpdate, *this, true);
}

// ##################################################################################################
/// @brief This is a callback function/method, called when "prices" event is fired (webhook got data)
// The entire message is broken up into 512 byte parts. Last part is less than 512 byte.
// ##################################################################################################
void PriceClass::pricesSubscriptionHandler(const char *pricesEvent, const char *data)
{
    messageDataReady = false;

    // Put the package number- ("prices/X") and data- key-value pairs in the messageParts map.
    // (note: ordered map means data is in their right place)
    messageParts[pricesEvent[7]] = data;

    if (strlen(data) < 512)
    {
        messageDataReady = true;
        newPricesReceived = false; // New prices not assembled yet.
        Serial.println("Message Data Received.");
    }
}

// ####################################################################################################
/// @brief This method assembles the messagesParts into one string, and seperates the hour/price parts.
// Message is structured in this format: !yyyy-mm-ddThh:mm:ss,pppp.pppppp!yyyy-mm.........
// The method saves the hours to the "currentHourPrices" vector, and also saves the hours that are
// below the chosen threshold in the "lowPriceHours" vector.
// ####################################################################################################
void PriceClass::assembleMessageData()
{
    if (!messageDataReady)
    {
        Serial.println("Message data isn't ready yet.");
        return;
    }

    std::string message = "";
    for (const auto &item : messageParts)
    {
        message.append(item.second);
    }

    Serial.println("Message assembled.");

    // Setup for while loop.
    message.erase(0, 1);                       // Erase the leading "bang" (!)
    uint8_t bangIndex = message.find("!") + 1; // End index of the first price part.
    currentHourPrices.clear();
    currentHighest.price = 0;
    currentLowest.price = 100;
    HourPrice indexedHourPrice;

    // Separate the prices, save in a vector of structs.
    while (bangIndex > 0)
    {
        indexedHourPrice.hour = stoi(message.substr(HOUR_INDEX, 2));                                      // Hour (hh) converted to int.
        indexedHourPrice.price = stof(message.substr(PRICE_INDEX, (bangIndex - 1) - PRICE_INDEX)) / 1000; // Price converted to float.
        currentHourPrices.push_back(indexedHourPrice);                                                    // Save.

        // check for highest and lowest price.
        if (indexedHourPrice.price > currentHighest.price)
            currentHighest = indexedHourPrice;
        if (indexedHourPrice.price < currentLowest.price)
            currentLowest = indexedHourPrice;

        message.erase(0, bangIndex);       // Erase the part we just read, ready for next iteration
        bangIndex = message.find("!") + 1; // Find the End index of next part.
    }

    // Calculate low price threshold
    double difference = currentHighest.price - currentLowest.price;
    double lowThreshold = currentLowest.price + difference * LOW_THRESHOLD_FACTOR;

    // Find hours of day at which price is below threshold.
    for (const auto &item : currentHourPrices)
    {
        if (item.price < lowThreshold)
        {
            lowPriceHours.push_back(item);
        }
    }

    // Clean up.
    messageDataReady = false;
    messageParts.clear();
    newPricesReceived = true;
}

// ###########################################################################
/// @brief This method returns a string of the saved low price hour intervals.
// ###########################################################################
std::string PriceClass::getLowPriceIntervals()
{
    if (!newPricesReceived)
        return "No prices yet";

    uint8_t lastIndexedHour = lowPriceHours[0].hour;
    std::string lowHourIntervals{(String)lastIndexedHour};

    for (const auto &item : lowPriceHours)
    {
        if (item.hour > lastIndexedHour + 1)
            lowHourIntervals.append(String::format(" to %02d, %02d", lastIndexedHour, item.hour));

        else if (item.hour < lastIndexedHour)
        {
            if (item.hour + 24 > lastIndexedHour + 1)
                lowHourIntervals.append(String::format(" to %02d, %02d", lastIndexedHour, item.hour));

            else
                lowHourIntervals.append(String::format(" to 24, %02d", item.hour));
        }
        lastIndexedHour = item.hour;
    }
    lowHourIntervals.append(String::format(" to %02d", lastIndexedHour));

    newPricesReceived = false;

    Serial.println("Low Price Intervals:");
    Serial.println(lowHourIntervals.c_str());

    return lowHourIntervals;
}

// ##################################################################
/// @brief This is a subscription handler to the "get_prices" event.
// ##################################################################
void PriceClass::getpricesSubscriptionHandler(const char *getpricesEvent, const char *data)
{
    requestPriceUpdate(Time.day());
}

// #####################################################################
/// @brief This method makes a publish request, to fire off the webhook,
// which in turn will fire off the "prices" event, when data is ready.
// #####################################################################
void PriceClass::requestPriceUpdate(int day)
{
    String data = String::format("{ \"year\": \"%d\", ", Time.year()) +
                  String::format("\"month\": \"%02d\", ", Time.month()) +
                  String::format("\"day\": \"%02d\", ", day) +
                  String::format("\"day_two\": \"%02d\", ", (day + 2)) +
                  String::format("\"hour\": \"%02d\" }", Time.hour());

    // Trigger the integration
    Particle.publish("elpriser", data);
    lastUpdate = Time.timeStr();
}

// ##################################################################
/// @brief Call this method in setup, to initialize the subscriptions
// ##################################################################
void PriceClass::initSubscriptions()
{
    Particle.subscribe("prices", &PriceClass::pricesSubscriptionHandler, this);
    Particle.subscribe("get_prices", &PriceClass::getpricesSubscriptionHandler, this);
    updatePrices();
    timedUpdate();
}

// ####################################################################
/// @brief This is a public method, for requesting an update on prices.
// ####################################################################
void PriceClass::updatePrices()
{
    requestPriceUpdate(Time.day());
}

// ###################################################################
/// @brief This method will aim to call itself around 11:00 and 23:00,
// requesting an update on prices.
// Doing this instead of a set repeating timer, since it doesn't
// correct its errors over time.
// ###################################################################
void PriceClass::timedUpdate()
{
    uint timerPeriod;
    uint8_t hourNow = Time.hour();
    if (hourNow == 11 || hourNow == 23)
    {
        timerPeriod = (12 * MILLISEC_HOUR) - (Time.minute() * MILLISEC_MINUTE);
        timedCountDown(timerPeriod);
        requestPriceUpdate(Time.day());
    }
    else
    {
        if (hourNow < 11)
        {
            timerPeriod = ((11 - hourNow) * MILLISEC_HOUR) - (Time.minute() * MILLISEC_MINUTE);
            timedCountDown(timerPeriod);
        }
        else
        {
            timerPeriod = ((23 - hourNow) * MILLISEC_HOUR) - (Time.minute() * MILLISEC_MINUTE);
            timedCountDown(timerPeriod);
        }
    }
}

// ##########################################################################
/// @brief Small method for setting the timer period, and starting the timer.
// ##########################################################################
void PriceClass::timedCountDown(uint period)
{
    updateTimer->stop();
    updateTimer->changePeriod(period);
    updateTimer->start();
}

// ########################################################################
/// @brief Returns the lastUpdate string, which holds the time for the last
// price update in the format: "Wed May 21 01:08:47 2014"
// ########################################################################
std::string PriceClass::getLastUpdate()
{
    return lastUpdate;
}

// #######################################################
/// @brief Return a copy of the whole lowPriceHours vector
// #######################################################
std::vector<PriceClass::HourPrice> PriceClass::getlowPriceHours()
{
    return lowPriceHours;
}

// ####################################################################
/// @brief Method for checking if new price data has not yet been read.
// ####################################################################
bool PriceClass::pricesUpdated()
{
    return newPricesReceived;
}

bool PriceClass::isMessageDataReady()
{
    return messageDataReady;
}