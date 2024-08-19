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
  void loop();
  bool isAvailable();

  void showPPO2(float sensor1ppO2, float sensor2ppO2, float sensor3ppO2);
  void showDisplayMessage(String message);

  private:
  bool _isAvailable;

  Adafruit_SSD1306 *_display;
  logging::Logger *_logger;
};

#endif