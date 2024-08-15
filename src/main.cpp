#include <Arduino.h>
#include <Wire.h>

#include <logger.h>

#include <RTClib.h>
#include <Adafruit_ADS1X15.h>

#include "Display.h"
#include "RTCTime.h"

#include "ISensor.h"
#include "O2VoltageSensor.h"

logging::Logger logger;


Display display(&logger);

RTC_DS3231 rtc;
RTCTime rtcTime(&logger, &rtc);

Adafruit_ADS1115 ads;
ISensor<float> *o2VoltageSensor1 = new O2VoltageSensor(&logger, &ads, 0);
ISensor<float> *o2VoltageSensor2 = new O2VoltageSensor(&logger, &ads, 1);
ISensor<float> *o2VoltageSensor3 = new O2VoltageSensor(&logger, &ads, 2);

void setup() {
  Serial.begin(115200);
  logger.setSerial(&Serial);

  display.setup();
  rtcTime.setup();

  o2VoltageSensor1->setup();
  o2VoltageSensor2->setup();
  o2VoltageSensor3->setup();
}

float generateRandom(float lower, float upper)
{
  float random = ((float)rand()) / (float)RAND_MAX;
  float diff = upper - lower;
  float r = random * diff;
  return lower + r;
}

bool randomBool() {
  return rand() > (RAND_MAX / 2);
}

class Voltage {
public:
  Voltage(float value) : _value(value), _reliable(isValid()) {}

  bool isValid() const {
    return _value >= 0 && _value <= 99.99;
  }

  bool isReliable() const {
    return _reliable;
  }

  void setReliable(bool reliability) {
    _reliable = reliability;
  }

  float getValue() const {
    return _value;
  }

private:
  float _value;
  bool _reliable;
};


float calculateAverage(Voltage& voltage1, Voltage& voltage2, Voltage& voltage3) {
  // Initialize an array to hold the valid voltages
  Voltage* validVoltages[3];
  int validCount = 0;

  // Add the valid voltages to the array
  if(voltage1.isValid()) validVoltages[validCount++] = &voltage1;
  if(voltage2.isValid()) validVoltages[validCount++] = &voltage2;
  if(voltage3.isValid()) validVoltages[validCount++] = &voltage3;

  // If there are no valid voltages, return an error value
  if(validCount == 0) return -1;

  // If there's only one valid voltage, return it
  if(validCount == 1) return validVoltages[0]->getValue();

  // Sort the valid voltages in ascending order
  for(int i = 0; i < validCount; i++) {
    for(int j = i + 1; j < validCount; j++) {
      if(validVoltages[i]->getValue() > validVoltages[j]->getValue()) {
        Voltage* temp = validVoltages[i];
        validVoltages[i] = validVoltages[j];
        validVoltages[j] = temp;
      }
    }
  }

  // If there are two valid voltages, return their average
  if(validCount == 2) return (validVoltages[0]->getValue() + validVoltages[1]->getValue()) / 2;

  // If there are three valid voltages, calculate the average of the two closest
  float avgClosest = (validVoltages[0]->getValue() + validVoltages[1]->getValue()) / 2;

  // If the farthest voltage deviates from the average of the two closest by less than 0.2, include it in the average
  if(abs(validVoltages[2]->getValue() - avgClosest) < 0.2) {
    validVoltages[2]->setReliable(true);
    return (validVoltages[0]->getValue() + validVoltages[1]->getValue() + validVoltages[2]->getValue()) / 3;
  } else {
    return avgClosest;
  }
}

#define ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL 1.01325 // SW does 1016mb

float calibrationVoltage = 52.5;

float getPPO2FromVoltage(float currentCellVoltage) {
  return ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL * currentCellVoltage / calibrationVoltage;
}

void loop() {
  o2VoltageSensor1->loop();
  o2VoltageSensor2->loop();
  o2VoltageSensor3->loop();

  float o2Sensor1Voltage = o2VoltageSensor1->read();
  float o2Sensor2Voltage = o2VoltageSensor2->read();
  float o2Sensor3Voltage = o2VoltageSensor3->read();

  Voltage voltage1(o2Sensor1Voltage);
  Voltage voltage2(o2Sensor2Voltage);
  Voltage voltage3(o2Sensor3Voltage);

  float avgVoltage = calculateAverage(voltage1, voltage2, voltage3);
  
  display.showReliantSensors(voltage1.isReliable(), voltage2.isReliable(), voltage3.isReliable());

  rtcTime.serialPrintDateTime();
  Serial.print(" - ");
  Serial.print("(");
  Serial.print(o2Sensor1Voltage);
  if (voltage1.isReliable()) {
    Serial.print(" ✅");
  }
  if (!voltage1.isReliable()) {
    Serial.print(" ❌");
  }
  Serial.print(")");
  Serial.print(" ");
  Serial.print("(");
  Serial.print(o2Sensor2Voltage);
  if (voltage2.isReliable()) {
    Serial.print(" ✅");
  }
  if (!voltage2.isReliable()) {
    Serial.print(" ❌");
  }
  Serial.print(")");
  Serial.print(" ");
  Serial.print("(");
  Serial.print(o2Sensor3Voltage);
  if (voltage3.isReliable()) {
    Serial.print(" ✅");
  }
  if (!voltage3.isReliable()) {
    Serial.print(" ❌");
  }
  Serial.print(")");
  Serial.print(" - Average: ");
  Serial.println(avgVoltage);

  // display.showPPO2(
  //   avgVoltage,
  //   o2Sensor2Voltage,
  //   o2Sensor3Voltage
  // );

  // display.showReliantSensors(
  //   voltage1.isReliable(),
  //   voltage2.isReliable(),
  //   voltage3.isReliable()
  // );

   display.showPPO2v2(
    getPPO2FromVoltage(o2Sensor1Voltage),
    getPPO2FromVoltage(o2Sensor2Voltage),
    getPPO2FromVoltage(o2Sensor3Voltage)
  );

  display.loop();
}