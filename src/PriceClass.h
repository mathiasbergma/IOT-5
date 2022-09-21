#pragma once

#include "application.h"

#define EVENT_NAME "elpriser"
#define INITIAL_RANGE 48
#define MAX_TRANSMISSIONS 5
#define MAX_TRANSMISSION_SIZE 512


class PriceClass
{
  private:
    unsigned long last_read;
    uint8_t transmission_count;
    int range = 48;
    int index = 0;
    int cnt;
    char temp[5 * 513];
    int power;
    bool printer = false;
    bool message_complete;
    char received_data[5][513];
    std::string message;
    double cost[48];   // vi requester 48 værdier. (det er max 36)
    int cost_hour[48]; // følger cost
    int date;

  public:
    // Constructor
    PriceClass();   // No-Args constructor

    void update_power();

    void request_price_data(int day);   // was: get_data

    void prices_subscription_handler(const char *event_name, const char *data);

};