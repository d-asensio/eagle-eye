#include "O2VoltageSensor.h"

#ifndef CALIBRATED_PPO2_READ_H
#define CALIBRATED_PPO2_READ_H

#define ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL 1.01325

class CalibratedPPO2Read {
public:
    CalibratedPPO2Read(O2VoltageSensor* sensor, float* calibrationReferenceVoltage);

    void takeCalibrationReference();
    float getPPO2();

private:
    O2VoltageSensor* _sensor;
    float* _calibrationReferenceVoltage;
};

#endif
