#include <Arduino.h>
#include <RTClib.h>

#include "logger.h"

#ifndef RTC_TIME_H
#define RTC_TIME_H

class RTCTime {
  public:
  RTCTime(logging::Logger *logger, RTC_DS3231 *_rtc);

  void setup();
  bool isAvailable();

  void initDateTimeModule();
  void serialPrintDateTime();

  private:
  bool _isAvailable;

  logging::Logger *_logger;
  RTC_DS3231 *_rtc;
};

#endif