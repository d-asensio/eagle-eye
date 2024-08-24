#include "O2VoltageSensor.h"

O2VoltageSensor::O2VoltageSensor(logging::Logger *logger, Adafruit_ADS1115 *ads, uint8_t channel)
{
  _isAvailable = false;
  _lastReading = 0;
  _channel = channel;

  _ads = ads;
  _logger = logger;
}

void O2VoltageSensor::setup()
{
  if (!_ads->begin())
  {
    _logger->log(
        logging::LoggerLevel::LOGGER_LEVEL_ERROR,
        "O2_VOLTAGE_SENSOR",
        "Error initializing the sensor");
    return;
  }

  _isAvailable = true;

  _readChannel();
}

void O2VoltageSensor::loop()
{
  if (!isAvailable())
    return;

  _readChannel();
}

bool O2VoltageSensor::isAvailable()
{
  return _isAvailable;
}

float O2VoltageSensor::read()
{
  return _lastReading * 0.1875F;
}

void O2VoltageSensor::_readChannel()
{
  if (!isAvailable())
    return;

  if (_channel == 0)
  {
    _lastReading = _ads->readADC_Differential_0_3();
  }

  if (_channel == 1)
  {
    _lastReading = _ads->readADC_Differential_1_3();
  }

  if (_channel == 2)
  {
    _lastReading = _ads->readADC_Differential_2_3();
  }
}