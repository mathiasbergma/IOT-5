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
    message.clear();

    // Set up the data string
    String data = String::format("{ \"year\": \"%d\", ", Time.year()) +
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


/* When transmission are greater than 512 bytes, it will be split into 51
* byte parts. The final transmission part should therefore be less than 512.
*/
void PriceClass::prices_subscription_handler(const char *event_name, const char *data) 
{
    message_complete = false;       //populate
    // Increment transmission counter:  //rec_cnt++;
    transmission_count++;
    
    // Save transmission size into variable so we can act on it
    int transmission_size = strlen(data);

    // "eventname/<transmission part no>"
    //char event_str[12];
    //strcpy(event_str, event_name);

    // Extract the numbered part of eventname and use it for indexing "rec_data"
    //strcpy(rec_data[atoi(strtok(event_str, "prices/"))], data);

    // copy event data to the Recieved Data array:
    strcpy(received_data[transmission_count - 1], data );

   message.append(data);

    // If transmission size is less than 512 = last transmission received
    if (transmission_size < 512)
    {
        message_complete = true;

        // Concatenate all transmission into one string
        for (int i = 0; i <= transmission_count; i++)
        {
            strcat(temp, received_data[i]);
        }

        // Token used for strtok()
        char *token = NULL;

        // Tokenize the string. i.e. split the string so we can get to the data
        token = strtok(temp, ",!");
        for (int i = 0; i < range; i++)
        {
            // Save hour and cost in differen containers
            sscanf(token, "%*d-%*d-%dT%d:%*d:%*d", &date, &cost_hour[i]);
            token = strtok(NULL, ",!");
            cost[i] = atof(token) / 1000;

            if ((token = strtok(NULL, ",!")) == NULL) // Received data count is less than 48.
            {
                range = i; // Update range, such that the rest of program flow is aware of size
                break;     // Break the while loop
            }
        }

        // Message i structured in this format: !yyyy-mm-ddThh:mm:ss,pppp.pppppp
        // So first a "!" before the date, and then a "T" before the time, and finally a "," before the price.
        // Here we index them, and use it to extract the useful info.
        
        message.erase(0,1);                 // Erase the leading "!"

        int dd_idx = message.find("T") - 2; // Index of the date
        int hh_idx = dd_idx + 3;            // Index of the hour
        int pp_idx = hh_idx + 9;            // Index of the price

        int bng_idx = message.find("!");    // Index of the end of the first part.
        int idx = 0;                        // cost / cost_hour indexing.

        // Iterate over the message, extracting the juicy bits.
        while (bng_idx != string::npos)
        {   
            date = stoi(message.substr(dd_idx, 2));                             // Date (dd) converted to int (string-to-int)
            cost_hour[idx] = stoi(message.substr(hh_idx, 2));                   // Hour (hh) converted to int. 
            cost[idx] = stof(message.substr(pp_idx, bng_idx - pp_idx)) / 1000;  // Price converted to float.

            message.erase(0, bng_idx);          // Erase the part we read, ready for next iteration           
            int bng_idx = message.find("!");    // Find index of next "!".
            idx++;                              // Increment array index.
        } 
        
    }
 }