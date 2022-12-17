#pragma once

extern double *cost_yesterday;
extern double *cost_today;
extern double *cost_tomorrow;
extern int *wh_yesterday;
extern int *wh_today;
extern String wh_today_Json;
extern String wh_yesterday_Json;
extern String pricestoday_Json;
extern String pricesyesterday_Json;
extern String pricestomorrow_Json;

void hourly_JSON_update()
{
    // Update wh_today_Json
    wh_today_Json = "{\"WHr_today\":[";
    for (int i = 0; i < 24; i++)
    {
        wh_today_Json += String(wh_today[i]);
        if (i < 23)
        {
            wh_today_Json += ",";
        }
    }
    wh_today_Json += "]}";
}

void update_JSON()
{
    // Update the json strings
    pricesyesterday_Json = String::format("{\"pricesyesterday\":[");
    for (int i = 0; i < 24; i++)
    {
        pricesyesterday_Json += String::format("%.2lf", cost_yesterday[i]);
        if (i < 23)
        {
            pricesyesterday_Json += String::format(",");
        }
    }
    pricesyesterday_Json += String::format("]}");

    // Updating prices today JSON string
    pricestoday_Json = String::format("{\"pricestoday\":[");
    for (int i = 0; i < 24; i++)
    {
        pricestoday_Json += String::format("%.2lf", cost_today[i]);
        if (i < 23)
        {
            pricestoday_Json += String::format(",");
        }
    }
    pricestoday_Json += String::format("]}");

    // Updating prices today JSON string
    pricestomorrow_Json = String::format("{\"pricestomorrow\":[");
    for (int i = 0; i < 24; i++)
    {
        pricestomorrow_Json += String::format("%.2lf", cost_tomorrow[i]);
        if (i < 23)
        {
            pricestomorrow_Json += String::format(",");
        }
    }
    pricestomorrow_Json += String::format("]}");

    // Updating watt hours used yesterday JSON string
    wh_yesterday_Json = String::format("{\"WHr_yesterday\":[");
    for (int i = 0; i < 24; i++)
    {
        wh_yesterday_Json += String::format("%d", wh_yesterday[i]);
        if (i < 23)
        {
            wh_yesterday_Json += String::format(",");
        }
    }
    wh_yesterday_Json += String::format("]}");

    // Updating watt hours for today JSON string
    wh_today_Json = String::format("{\"WHr_today\":[");
    for (int i = 0; i < 24; i++)
    {
        wh_today_Json += String::format("%d", wh_today[i]);
        if (i < 23)
        {
            wh_today_Json += String::format(",");
        }
    }
    wh_today_Json += String::format("]}");
}

String update_Whr_Today_JSON()
{
    // Updating watt hours for today JSON string
    wh_today_Json = String::format("{\"WHr_today\":[");
    for (int i = 0; i < 24; i++)
    {
        wh_today_Json += String::format("%d", wh_today[i]);
        if (i < 23)
        {
            wh_today_Json += String::format(",");
        }
    }
    wh_today_Json += String::format("]}");

    return wh_today_Json;
}