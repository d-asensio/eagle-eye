#include <Arduino.h>
#include <Wire.h>

#include <logger.h>
#include <TaskScheduler.h>

#include <RTClib.h>
#include <Adafruit_ADS1X15.h>

#include <AceButton.h>
#include "driver/touch_sensor.h"

#include "Display.h"
#include "RTCTime.h"

#include "ISensor.h"
#include "O2VoltageSensor.h"

#define ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL 1.01325 // SW does 1.016mb

#define BUDDY_LED_R_PIN (18)
#define BUDDY_LED_G_PIN (19)
#define BUDDY_LED_B_PIN (17)

#define WET_CONTACT_PIN (26)

#define PIEZO_ELECTRIC_SWITCH_PIN (33)

#define BUZZER_PIN (12)

#define TOUCHPAD_FILTER_TOUCH_PERIOD (30)
#define TOUCHPAD_TOUCH_THRESHOLD (200)

logging::Logger logger;
Scheduler scheduler;

using namespace ace_button;

class CapacitiveConfig : public ButtonConfig
{
public:
  CapacitiveConfig(touch_pad_t touchPad) : mTouchPad(touchPad) {}

protected:
  int readButton(uint8_t /*pin*/) override
  {
    touch_value_t touchPadValue;
    touch_pad_read_raw_data(mTouchPad, &touchPadValue);

    // Serial.println(touchPadValue);

    return touchPadValue < TOUCHPAD_TOUCH_THRESHOLD ? LOW : HIGH;
  }

private:
  touch_pad_t mTouchPad;
};

void handleEvent(AceButton * /* button */, uint8_t eventType,
                 uint8_t /* buttonState */)
{
  switch (eventType)
  {
  case AceButton::kEventPressed:
    logger.log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "TOUCH_SENSOR",
        "Pressed");
    break;
  case AceButton::kEventClicked:
    logger.log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "TOUCH_SENSOR",
        "Clicked");
    break;
  case AceButton::kEventDoubleClicked:
    logger.log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "TOUCH_SENSOR",
        "Double Clicked");
    break;
  }
}

CapacitiveConfig buttonConfig(TOUCH_PAD_NUM2);
AceButton button(&buttonConfig);

Display display(&logger);

RTC_DS3231 rtc;
RTCTime rtcTime(&logger, &rtc);

Adafruit_ADS1115 ads;
ISensor<float> *o2VoltageSensor1 = new O2VoltageSensor(&logger, &ads, 0);
ISensor<float> *o2VoltageSensor2 = new O2VoltageSensor(&logger, &ads, 1);
ISensor<float> *o2VoltageSensor3 = new O2VoltageSensor(&logger, &ads, 2);

float calibrationVoltage = 52.5;

float getPPO2FromVoltage(float currentCellVoltage)
{
  return ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL * currentCellVoltage / calibrationVoltage;
}

void t1Callback() {
  Serial.print("t1: ");
  Serial.println(millis());
}

Task t1(2000, 10, &t1Callback);

void setup()
{
  Serial.begin(115200);
  logger.setSerial(&Serial);
  
  scheduler.init();
  scheduler.addTask(t1);
  
  t1.enable();

  // Initialize touch pad peripheral.
  touch_pad_init();
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_config(TOUCH_PAD_NUM2, TOUCHPAD_TOUCH_THRESHOLD);
  touch_pad_intr_enable();

  // Initialize and start a software filter to detect valid touch pad events
  touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);

  // Configure the button using CapacitiveConfig.
  buttonConfig.setFeature(ButtonConfig::kFeatureClick);
  buttonConfig.setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig.setEventHandler(handleEvent);

  display.setup();
  rtcTime.setup();

  o2VoltageSensor1->setup();
  o2VoltageSensor2->setup();
  o2VoltageSensor3->setup();

  pinMode(BUDDY_LED_R_PIN, OUTPUT);
  pinMode(BUDDY_LED_G_PIN, OUTPUT);
  pinMode(BUDDY_LED_B_PIN, OUTPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(WET_CONTACT_PIN, INPUT_PULLDOWN);

  pinMode(PIEZO_ELECTRIC_SWITCH_PIN, INPUT_PULLDOWN);
}

void loop()
{
  o2VoltageSensor1->loop();
  o2VoltageSensor2->loop();
  o2VoltageSensor3->loop();

  float o2Sensor1Voltage = o2VoltageSensor1->read();
  float o2Sensor2Voltage = o2VoltageSensor2->read();
  float o2Sensor3Voltage = o2VoltageSensor3->read();

  // rtcTime.serialPrintDateTime();
  // Serial.print(" - ");

  // Serial.print("(");
  // Serial.print(o2Sensor1Voltage);
  // Serial.print(")");

  // Serial.print(" ");

  // Serial.print("(");
  // Serial.print(o2Sensor2Voltage);
  // Serial.print(")");

  // Serial.print(" ");

  // Serial.print("(");
  // Serial.print(o2Sensor3Voltage);
  // Serial.println(")");

  display.showPPO2(
      getPPO2FromVoltage(o2Sensor1Voltage),
      getPPO2FromVoltage(o2Sensor2Voltage),
      getPPO2FromVoltage(o2Sensor3Voltage));

  // ---

  boolean isWet = digitalRead(WET_CONTACT_PIN) == HIGH;

  if (isWet) {
    analogWrite(BUDDY_LED_R_PIN, 0);
    analogWrite(BUDDY_LED_G_PIN, 0);
    analogWrite(BUDDY_LED_B_PIN, 255);
  } else {
    analogWrite(BUDDY_LED_R_PIN, 255);
    analogWrite(BUDDY_LED_G_PIN, 0);
    analogWrite(BUDDY_LED_B_PIN, 0);
  }

  // ----

  boolean isPiezoActive = digitalRead(PIEZO_ELECTRIC_SWITCH_PIN) == HIGH;

  if (isPiezoActive) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // ----

  display.loop();

  // ----

  scheduler.execute();

  // ----

  unsigned long start = millis();
  button.check();

  // check on performance in milliseconds
  unsigned long duration = millis() - start;
  if (duration > 10)
  {
    Serial.print("duration: ");
    Serial.println(duration);
  }
}