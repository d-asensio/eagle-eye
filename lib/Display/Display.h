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
  void showPPO2v2(float sensor1ppO2, float sensor2ppO2, float sensor3ppO2);
  void showReliantSensors(boolean sensor1IsReliant, boolean sensor2IsReliant, boolean sensor3IsReliant);
  void showDisplayMessage(String message);
  void showDisplayGasInformation(float o2SensorVoltage, float percentageO2);


  private:
  bool _isAvailable;
  int _direction;
  int _x; 

  Adafruit_SSD1306 *_display;
  logging::Logger *_logger;
};

#endif