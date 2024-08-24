#include "CalibratedPPO2Read.h"

CalibratedPPO2Read::CalibratedPPO2Read(O2VoltageSensor* sensor, float* calibrationReferenceVoltage)
{
    _sensor = sensor;
    _calibrationReferenceVoltage = calibrationReferenceVoltage;
}

void CalibratedPPO2Read::takeCalibrationReference()
{
    *_calibrationReferenceVoltage = _sensor->read();
}

float CalibratedPPO2Read::getPPO2()
{
    return ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL * _sensor->read() / *_calibrationReferenceVoltage;
}
