#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

#include "logger.h"

#ifndef DISPLAY_H
#define DISPLAY_H

class Display {
  public:
  Display(logging::Logger *logger);

  void setup();
  bool isAvailable();

  void showSensorPPO2(float ppO2, uint8_t sensorChannel);
  void showCenteredMessage(String message);

  private:
  bool _isAvailable;

  Adafruit_SSD1306 *_display;
  logging::Logger *_logger;
};

#endif