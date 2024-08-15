#include "RTCTime.h"

RTCTime::RTCTime(logging::Logger *logger, RTC_DS3231 *rtc) {
  _logger = logger;
  _rtc = rtc;

  _isAvailable =  false;
}

void RTCTime::setup() {
  if (!_rtc->begin()) {
    _logger->log(
            logging::LoggerLevel::LOGGER_LEVEL_ERROR,
            "Clock",
            "Unable to initialize the RTC module");
    return;
  }

  _isAvailable =  true;

  _logger->log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "Clock",
        "Clock initialized");


#if CONFIG_RESET_DATETIME == 1
  _rtc->adjust(DateTime(F(__DATE__), F(__TIME__)));

_logger->log(
          logging::LoggerLevel::LOGGER_LEVEL_WARN,
          "Clock",
          "Date and time have been resetted, remember to set CONFIG_RESET_DATETIME to 0 and reupload the firmware!");    
#endif
}

bool RTCTime::isAvailable () {
  return _isAvailable;
}

void RTCTime::serialPrintDateTime () {
    if (!isAvailable()) return;

    DateTime now = _rtc->now();

    Serial.print("RTC Date Time: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print('-');
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
}