#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include "logger.h"

#ifndef DISPLAY_H
#define DISPLAY_H

#define WATER_DROP_WIDTH 8
#define WATER_DROP_HEIGHT 8

static const unsigned char PROGMEM water_drop_bmp[] =
{ 
  B00010000,
  B00111000,
  B01111100,
  B01111100,
  B11111110,
  B11111110,
  B11111110,
  B01111100,
  B00111000
};

class Display {
  public:
  Display(logging::Logger *logger);

  void setup();
  bool isAvailable();

  void showSensorPPO2(float ppO2, uint8_t sensorChannel, bool showWetSymbol);
  void showCenteredMessage(String message);
  void off();

  private:
  bool _isAvailable;

  Adafruit_SSD1306 *_display;
  logging::Logger *_logger;
};

#endif