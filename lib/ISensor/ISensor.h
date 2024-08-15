#ifndef I_SENSOR_H
#define I_SENSOR_H

template <typename V>

class ISensor {
public:
    virtual ~ISensor() = default;
    virtual void setup() = 0;
    virtual void loop() = 0;
    virtual V read() = 0;
    virtual bool isAvailable() = 0;
};

#endif