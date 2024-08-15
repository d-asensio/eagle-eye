#include "ISensor.h"
#include "logger.h"

#include <Adafruit_ADS1X15.h>

#ifndef O2_VOLTAGE_SENSOR_H
#define O2_VOLTAGE_SENSOR_H

class O2VoltageSensor : public ISensor<float> {
public:
    O2VoltageSensor(logging::Logger* logger, Adafruit_ADS1115* ads,  uint8_t channel);

    virtual void setup();
    virtual void loop();
    virtual bool isAvailable();
    virtual float read();
private:
    uint8_t _channel;
    bool _isAvailable;
    float _lastReading;

    void _readChannel();

    Adafruit_ADS1115* _ads;
    logging::Logger* _logger;
};

#endif