/*
 Example animated analogue meters using a TFT LCD screen

 Originanally written for a 320 x 240 display, so only occupies half
 of a 480 x 320 display.

 Needs Font 2 (also Font 4 if using large centered scale label)

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
 */
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Arduino_JSON.h>

#include <TFT_eSPI.h> // Hardware-specific library
#include "defines.h"
#include "BLE_include.h"

int whr_count_now;
int last_hr;

// prototypes
int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme);
int update_ringMeter(int value_last, int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme);
unsigned int rainbow(byte value);
void graph(int originX, int originY, volatile double values[24], int sizeX, int sizeY, int Maxvalue);

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

int reading = 0; // Value to be displayed
int last_reading = 0;
int d = 0; // Variable used for the sinewave test waveform

void setup(void)
{
  Serial.begin(115200);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillRect(120, 120, 240, 100, TFT_GREY);
  tft.drawString("Starting bluetooth please wait...", 140, 160);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(10);
  connect_argonpm();

  tft.fillScreen(TFT_BLACK);
  tft.drawString("bluetooth Power monitor found   ", 140, 160);
  
  vTaskDelay(500 / portTICK_PERIOD_MS);
  tft.fillScreen(TFT_BLACK);

  int xpos = 5, ypos = 5, radius = 50;
  reading = 1750;
  // Comment out above meters, then uncomment the next line to show large meter
  last_reading = ringMeter(reading, 0, 2000, xpos, ypos, radius, "Watts", GREEN2RED);

  
}

void loop()
{
  if (!pClient->isConnected())
  {
    Serial.println("connection lost looking for new connection");
    connect_argonpm();
  }
  vTaskDelay(20 / portTICK_PERIOD_MS);

  // Draw a large meter
  int xpos = 5, ypos = 5, radius = 50;
  reading = power;
  // Comment out above meters, then uncomment the next line to show large meter
  last_reading = update_ringMeter(last_reading, reading, 0, 2000, xpos, ypos, radius, "Watts", GREEN2RED); // Draw analogue meter
  
  //graph on bottom
  int originX = 5;
  int originY = 310;
  int sizeY = 60;
  int sizeX = 470;
  graph(originX, originY, &pricetoday[0], sizeX, sizeY, maxtoday);

  //graph on top
  originX = 5;
  originY = 240;
  sizeY = 60;
  sizeX = 470;
  graph(originX, originY, &WHrToday_arr[0], sizeX, sizeY, 24);
}


void graph(int originX, int originY, volatile double values[24], int sizeX, int sizeY, int Maxvalue){
  tft.setCursor(10, 10); // set the cursor
  int posBlock[24];
  int ddd[24];
  int graphRange = Maxvalue;
  /*
  int originX = 5;
  int originY = 300;
  int sizeY = 50;
  int sizeX = 470;*/
  int numberOfMarks = 24;
  int boxSize = (sizeX / numberOfMarks);
  for(int i = 0; i < 24; i++)
  {
    //posBlock[i] = map(values[i], 0, graphRange, originY, (originY - sizeY));
    posBlock[i] = map(values[i], 0, graphRange, originY, (originY - sizeY));
    ddd[i] = map(values[i],0, graphRange, sizeY,0);
  }

   // draw the blocks - draw only if value differs
  for(int i = 0; i < 24; i++)
  {
      tft.fillRect(originX+(i * boxSize), (originY - sizeY), (boxSize ), (ddd[i]), TFT_GREY);
      delay(10);
      tft.fillRect(originX+(i * boxSize), posBlock[i], (boxSize - 1), (originY - posBlock[i]), TFT_GREEN);
    
    char buf[4]; 
    sprintf(buf,"%d", i);
    tft.drawCentreString(buf, originX+ (boxSize/2)+boxSize*i, originY, 1);
  }

}
// #########################################################################
//  Draw the meter on the screen, returns x coord of righthand side
// #########################################################################
int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option

  x += r;
  y += r; // Calculate coords of centre of ring

  int w = r / 4; // Width of outer ring is 1/4 of radius

  int angle = 150; // Half the sweep angle of meter (300 degrees)

  int text_colour = 0; // To hold the text colour

  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 5; // Segments are 5 degrees wide = 60 segments for 300 degrees
  byte inc = 5; // Draw segments every 5 degrees, increase to 10 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = -angle; i < angle; i += inc)
  {

    // Choose colour from scheme
    int colour = 0;
    switch (scheme)
    {
    case 0:
      colour = TFT_RED;
      break; // Fixed colour
    case 1:
      colour = TFT_GREEN;
      break; // Fixed colour
    case 2:
      colour = TFT_BLUE;
      break; // Fixed colour
    case 3:
      colour = rainbow(map(i, -angle, angle, 0, 127));
      break; // Full spectrum blue to red
    case 4:
      colour = rainbow(map(i, -angle, angle, 63, 127));
      break; // Green to red (high temperature etc)
    case 5:
      colour = rainbow(map(i, -angle, angle, 127, 63));
      break; // Red to green (low battery etc)
    default:
      colour = TFT_BLUE;
      break; // Fixed colour
    }

    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v)
    { // Fill in coloured segments with 2 triangles
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREY);
    }
  }

  // Convert value to a string
  char buf[10];
  byte len = 4;
  if (value > 999)
    len = 5;
  dtostrf(value, len, 0, buf);

  // Set the text colour to default
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Uncomment next line to set the text colour to the last segment value!
  // tft.setTextColor(text_colour, ILI9341_BLACK);

  // Print value, if the meter is large then use big font 6, othewise use 4
  if (r > 84)
    tft.drawCentreString(buf, x - 5, y - 20, 6); // Value in middle
  else
    tft.drawCentreString(buf, x - 5, y - 20, 4); // Value in middle

  // Print units, if the meter is large then use big font 4, othewise use 2
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (r > 84)
    tft.drawCentreString(units, x, y + 30, 4); // Units display
  else
    tft.drawCentreString(units, x, y + 5, 2); // Units display

  // Calculate and return right hand side x coordinate
  return value;
}

int update_ringMeter(int value_last, int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme)
{
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option

  x += r;
  y += r; // Calculate coords of centre of ring

  int w = r / 4; // Width of outer ring is 1/4 of radius

  int angle = 150; // Half the sweep angle of meter (300 degrees)

  int text_colour = 0; // To hold the text colour
  int target = value;
  if (value_last >= target)
  {
    if (value_last == target)
    {
      return value;
    }
    else if ((value_last - 20) < target)
    {
      value = target;
    }
    else
    {
      value = value_last - 20;
    }
  }
  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v

  byte seg = 5; // Segments are 5 degrees wide = 60 segments for 300 degrees
  byte inc = 5; // Draw segments every 5 degrees, increase to 10 for segmented ring

  // Draw colour blocks every inc degrees
  for (int i = -angle; i < angle; i += inc)
  {

    // Choose colour from scheme
    int colour = 0;
    switch (scheme)
    {
    case 0:
      colour = TFT_RED;
      break; // Fixed colour
    case 1:
      colour = TFT_GREEN;
      break; // Fixed colour
    case 2:
      colour = TFT_BLUE;
      break; // Fixed colour
    case 3:
      colour = rainbow(map(i, -angle, angle, 0, 127));
      break; // Full spectrum blue to red
    case 4:
      colour = rainbow(map(i, -angle, angle, 63, 127));
      break; // Green to red (high temperature etc)
    case 5:
      colour = rainbow(map(i, -angle, angle, 127, 63));
      break; // Red to green (low battery etc)
    default:
      colour = TFT_BLUE;
      break; // Fixed colour
    }

    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v)
    { // Fill in coloured segments with 2 triangles
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      text_colour = colour; // Save the last colour drawn
    }
    else // Fill in blank segments
    {
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, TFT_GREY);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, TFT_GREY);
    }
  }

  // Convert value to a string
  char buf[10];
  byte len = 4;
  if (value > 999)
    len = 5;
  dtostrf(value, len, 0, buf);
  char buf2[10];
  if (!(value_last == target))
  {
    len = 4;
    if (value_last > 999)
      len = 5;
    dtostrf(value_last, len, 0, buf2);

    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    if (r > 84)
      tft.drawCentreString(buf2, x - 5, y - 20, 6); // Value in middle
    else
      tft.drawCentreString(buf2, x - 5, y - 20, 4); // Value in middle
  }
  len = 4;
  if (value_last > 999)
    len = 5;
  dtostrf(value, len, 0, buf2);

  tft.setTextColor(TFT_BLACK, TFT_BLACK);
  if (r > 84)
    tft.drawCentreString(buf2, x - 5, y - 20, 6); // Value in middle
  else
    tft.drawCentreString(buf2, x - 5, y - 20, 4); // Value in middle

  // Set the text colour to default
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Uncomment next line to set the text colour to the last segment value!
  // tft.setTextColor(text_colour, ILI9341_BLACK);

  // Print value, if the meter is large then use big font 6, othewise use 4
  if (r > 84)
    tft.drawCentreString(buf, x - 5, y - 20, 6); // Value in middle
  else
    tft.drawCentreString(buf, x - 5, y - 20, 4); // Value in middle
                                                 /*
                                                   // Print units, if the meter is large then use big font 4, othewise use 2
                                                   tft.setTextColor(TFT_WHITE, TFT_BLACK);
                                                   if (r > 84) tft.drawCentreString(units, x, y + 30, 4); // Units display
                                                   else tft.drawCentreString(units, x, y + 5, 2); // Units display
                                                 */

  return value;
}
// #########################################################################
// Return a 16 bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0;   // Red is the top 5 bits of a 16 bit colour value
  byte green = 0; // Green is the middle 6 bits
  byte blue = 0;  // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0)
  {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1)
  {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2)
  {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3)
  {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}
