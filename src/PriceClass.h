#pragma once

#include "application.h"

#define EVENT_NAME "elpriser"


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

  public:
    // Constructor
    PriceClass();   // No-Args constructor

    void update_power();

    void request_price_data(int day);   // was: get_data

    void price_subscription_handler(const char *event_name, const char *data);

};