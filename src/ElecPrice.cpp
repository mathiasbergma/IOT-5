/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/mathi/Desktop/IOT/ElecPrice/src/ElecPrice.ino"
#include "string.h"

void setup();
void myHandler(const char *event, const char *data);
void myPriceHandler(const char *event, const char *data);
void loop();
#line 3 "c:/Users/mathi/Desktop/IOT/ElecPrice/src/ElecPrice.ino"
#define DELTA_OFFSET 0.3
#define KW_SENSOR_PIN D6
#define WATT_CONVERSION_CONSTANT 3600000

double cost[48];
int cost_hour[48];
int date;
int range = 48;         // Max received count. Updated if received count is smaller
char temp[5*513];       // Create an array that can hold the entire transmission
char rec_data[5][513];  // Array for holding individual parts of transmission
byte rec_cnt;           // Counter to keep track of recieved transmissions
bool populate = false;  // Entire transmission received flag
bool work = false;
bool printer = false;

// Variables used for finding highest and lowest price
double last_big = 0;
double last_small = 100; // Assign any absurdly high value
int big_idx;
int small_idx;
int low_range_hour[24];
int idx = -1;
double delta;
double small_offset;
int cnt;
int start_stop[12][2] = {0};

unsigned long last_read;
int calc_power;

void calc_low(void);
void get_data(int day);
void handle_sensor(void);

void setup()
{
    last_read = millis(); //Give it an initial value
    pinMode(KW_SENSOR_PIN, INPUT_PULLDOWN);
    attachInterrupt(KW_SENSOR_PIN,handle_sensor,RISING);
    
    // Particle.variable("Delta", delta);
    //Particle.variable("Delta_offset", small_offset);
    Particle.variable("Biggest", last_big);
    Particle.variable("Smallest", last_small);
    Particle.variable("Power", calc_power);
    
    get_data(Time.day());
    // Subscribe to the integration response event
    Particle.subscribe("prices", myHandler, MY_DEVICES);
    Particle.subscribe("get_prices", myPriceHandler, MY_DEVICES);
}

void handle_sensor(void)
{
    unsigned long delta;
    unsigned long current_reading = millis();
    //erial.printf("millis now: %ld\tlast millis: %ld\n",current_reading, last_read);
    if ((delta = current_reading-last_read) > 100)
    {
        calc_power = WATT_CONVERSION_CONSTANT / delta;
        last_read = current_reading;
        printer = true;
    }
}

void myHandler(const char *event, const char *data)
{
    populate = false;
    rec_cnt++;
    int transmission_size = strlen(data);
    char event_str[12];
    strcpy(event_str,event);

    //Serial.printf("%s size: %d",event,transmission_size);
    //Serial.printf("%s\n\n",data);

    // Token used for strtok()
    char *token = NULL;
    strcpy(rec_data[atoi(strtok(event_str,"prices/"))],data);
    if (transmission_size < 512)
    {
        populate = true;
    }

    if (populate)
    {
        // Mainly for debugging - Display what has been received
        // Serial.printf("%s\n\n",temp);
        for (int i = 0; i <= rec_cnt; i++)
        {
            strcat(temp,rec_data[i]);
            //Serial.printf("%s\n",temp);
        }
        // Tokenize the string
        token = strtok(temp, ",!");

        for (int i = 0; i < range; i++)
        {
            // Save hour and cost in differen containers
            sscanf(token, "%*d-%*d-%dT%d:%*d:%*d", &date, &cost_hour[i]);
            token = strtok(NULL, ",!");
            cost[i] = atof(token) / 1000;
            
            if((token = strtok(NULL, ",!")) == NULL)
            {
                range = i;
                //Serial.printf("Size of range: %d\n",range);
                break;
            }
        }
        //free(temp);
    }
}

void myPriceHandler(const char *event, const char *data)
{
    //fetch_price = true;
    get_data(Time.day());
}

void loop()
{
    if (printer)
    {
        Serial.printf("Light: %d\n",calc_power);
        printer = false;
    }
    // Has the prices for today arrived?
    if (populate)
    {
        calc_low();
        Serial.printf("Current HH:MM: %02d:%02d\n", Time.hour() + 2, Time.minute());
    }

    if (work)
    {
        // Do some meaningfull work with the collected data
        String data = "Cheap(ish) hours of the day: ";
        for (int z = 0; z <= cnt; z++)
        {
            data += String::format("%02d to %02d, ",start_stop[z][0],start_stop[z][1]);
        }
        // Trigger the integration
        Particle.publish("Low price hours", data, PRIVATE);
        work = false;
    }
    // Wait 10 seconds
    delay(5000);
}

void calc_low(void)
{
    for (int i = 0; i < range; i++)
    {
        // Find the highest price in range
        if (cost[i] > last_big)
        {
            big_idx = i;
            last_big = cost[i];
        }
        // Find the lowest price in range
        if (cost[i] < last_small)
        {
            // Serial.printf("Smallest before change[idx] %d:\t%f,\t cost: %f\n",small_idx,last_small,cost[i]);
            small_idx = i;
            last_small = cost[i];
            // Serial.printf("Current smallest[idx] %d:\t%f\n",small_idx,last_small);
        }
    }
    // Calculate delta
    delta = last_big - last_small;

    // Define low price area
    small_offset = last_small + delta * DELTA_OFFSET;

    // Find hours of day at which price is within the defined low price point
    for (int i = 0; i < range; i++)
    {
        if (cost[i] < small_offset)
        {
            idx++;
            low_range_hour[idx] = cost_hour[i];
            // Serial.printf("low_range_hour: %d\tidx: %d\n",low_range_hour[i],idx);
        }
    }

    // Calculations have been done - clear flag
    populate = false;
    // Display the results
    Serial.printf("Highest price of the day: %f\n", last_big);
    Serial.printf("Lowest price of the day: %f\n", last_small);
    Serial.printf("Hours of the day where electricity is within accepted range:\n");
    
    int i = 0;
    if (idx > 0)
    {
        while (i <= idx)
        {
            //Serial.printf("Step one %d\n",low_range_hour[i]);
            start_stop[cnt][0] = low_range_hour[i];
            //Serial.printf("Step two %d\n",start_stop[cnt][0]);
            while (low_range_hour[i] == low_range_hour[i + 1] - 1)
            {
                i++;
            }
            //Serial.printf("Step three %d\n",start_stop[cnt][1]);
            start_stop[cnt][1] = low_range_hour[i];
            //Serial.printf("Step four %d\n",start_stop[cnt][1]);
            cnt++;
            i++;
        }
        cnt--;
    }
    for (int z = 0; z <= cnt; z++)
    {
        Serial.printf("%02d to %02d\n",start_stop[z][0],start_stop[z][1]);
    }

    /*
    if (idx > 0)
    {
        start_stop[0][0] = low_range_hour[0];
        in_line = true;
    }
    for (int i = 0; i <= idx; i++)
    {
        if (i > 0)
        {
            if (low_range_hour[i] == low_range_hour[i - 1] + 1)
            {
                ;
            }
            else
            {
                start_stop[cnt][1] = low_range_hour[i - 1];
            }
        }

        Serial.printf("Hour: %d\tPrice: %f\n", low_range_hour[i], cost[low_range_hour[i]]);
    }
    */
    work = true;
}

void get_data(int day)
{
    rec_cnt = 0;
    range = 48;
    idx = 0;
    cnt = 0;
    temp[0] = 0;
    String data = String::format("{ \"year\": \"%d\", \"month\":\"%02d\", \"day\": \"%02d\", \"day_two\": \"%02d\", \"hour\": \"%02d\" }", Time.year(), Time.month(), day, day + 2, Time.hour());
    //Serial.printf("%s\n", data.c_str());
     // Trigger the integration
    Particle.publish("elpriser", data, PRIVATE);
}