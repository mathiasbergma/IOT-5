#ifndef COST_CALC_H
#define COST_CALC_H

#include "state_variables.h"
#define DELTA_OFFSET 0.3


/** @brief The purpose of the function is to identify the hours at which the highest and lowest cost are.
 *  Furthermore neighbouring low cost hour are identified and saved in an array for easy presentation
 * @param low_price_intervals 2D array into which to low price times are put
 * @param cost array of costs
*/
int calc_low(int low_price_intervals[12][2], double * cost, int size)
{
    int low_range_hour[24];
    int cnt = 0;
    int idx = 0;

    double delta;
    double small_offset;
    double last_big = 0.0;
    double last_small = 1000.0; // Assign any absurdly high value

    for (int i = 0; i < size; i++)
    {
        // Find the highest price in range
        if (cost[i] > last_big)
        {
            last_big = cost[i];
        }
        // Find the lowest price in range
        if (cost[i] < last_small)
        {
            last_small = cost[i];
        }
    }
    // Calculate delta
    delta = last_big - last_small;

    // Define low price area
    small_offset = last_small + delta * DELTA_OFFSET;
    
    // Find hours of day at which price is within the defined low price point
    for (int i = 0; i < size; i++)
    {
        
        if (cost[i] < small_offset)
        {
            low_range_hour[idx] = i;
            Serial.printf("low_range_hour[%d]: %d\n",idx, low_range_hour[idx]);
            idx++;
        }
    }

    // Display the results
    Serial.printf("Highest price of the day: %f\n", last_big);
    Serial.printf("Lowest price of the day: %f\n", last_small);
    Serial.printf("Hours of the day where electricity is within accepted range:\n");
    
    int i = 0;
    if (idx > 0)
    {
        while (i <= idx)
        {
            low_price_intervals[cnt][0] = low_range_hour[i];

            while (low_range_hour[i] == low_range_hour[i + 1] - 1) // Hour only increased by 1. I.e. coherant
            {
                i++;
            }
            
            low_price_intervals[cnt][1] = low_range_hour[i]+1;
            
            cnt++;
            i++;
        }
        cnt--;
    }
    for (int z = 0; z < cnt; z++)
    {
        Serial.printf("%02d to %02d\n",low_price_intervals[z][0],low_price_intervals[z][1]);
    }

    // Calculations are done - set flag
    TRANSMIT_PRICE = true;

    return cnt;
}
#endif