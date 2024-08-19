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

#define BUTTON_SWITCH_PIN (33)

#define BUZZER_PIN (25)

using namespace ace_button;

logging::Logger logger;
Scheduler scheduler;

AceButton button;

Display display(&logger);

RTC_DS3231 rtc;
RTCTime rtcTime(&logger, &rtc);

Adafruit_ADS1115 ads;
ISensor<float> *o2VoltageSensor1 = new O2VoltageSensor(&logger, &ads, 0);
ISensor<float> *o2VoltageSensor2 = new O2VoltageSensor(&logger, &ads, 1);
ISensor<float> *o2VoltageSensor3 = new O2VoltageSensor(&logger, &ads, 2);

float calibrationVoltage = 52.5;

uint8_t displaySensorChannel = 1;

float getPPO2FromVoltage(float currentCellVoltage)
{
  return ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL * currentCellVoltage / calibrationVoltage;
}

Task checkButtons(TASK_IMMEDIATE, TASK_FOREVER, []() {
  button.check();
}, &scheduler, true);

Task checkSensors(TASK_IMMEDIATE, TASK_FOREVER, []() {
  o2VoltageSensor1->loop();
  o2VoltageSensor2->loop();
  o2VoltageSensor3->loop();
}, &scheduler, true);

Task checkWetContact(TASK_IMMEDIATE, TASK_FOREVER, []() {
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
}, &scheduler, true);

Task endBeepTask(TASK_IMMEDIATE, 1, []() {
  digitalWrite(BUZZER_PIN, LOW);
}, &scheduler);

Task beginBeepTask(TASK_IMMEDIATE, 1, []() {
  digitalWrite(BUZZER_PIN, HIGH);
}, &scheduler);

Task showSensorsPpO2Task(TASK_IMMEDIATE, TASK_FOREVER, []() {
  if(displaySensorChannel == 3) {
    display.showSensorPPO2(
      getPPO2FromVoltage(o2VoltageSensor3->read()),
      displaySensorChannel
    );
    return;
  }

  if(displaySensorChannel == 2) {
    display.showSensorPPO2(
      getPPO2FromVoltage(o2VoltageSensor2->read()),
      displaySensorChannel
    );
    return;
  }

  if(displaySensorChannel == 1) {
    display.showSensorPPO2(
      getPPO2FromVoltage(o2VoltageSensor1->read()),
      displaySensorChannel
    );
    return;
  }
}, &scheduler, true);

Task rotateSensorsPpO2Task(1000, TASK_FOREVER, []() {
  displaySensorChannel++;

  if(displaySensorChannel > 3) {
    displaySensorChannel = 1;
  }
}, &scheduler, true);

void handleEvent(AceButton * /* button */, uint8_t eventType,
                 uint8_t /* buttonState */)
{
  switch (eventType)
  {
  case AceButton::kEventLongPressed:
    logger.log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "TOUCH_SENSOR",
        "Long Pressed");
    break;
  case AceButton::kEventPressed:
    logger.log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "TOUCH_SENSOR",
        "Pressed");
        beginBeepTask.restart();
        endBeepTask.restartDelayed(1000);
    break;
  case AceButton::kEventReleased:
    logger.log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "TOUCH_SENSOR",
        "Released");
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

void setup()
{
  Serial.begin(115200);
  logger.setSerial(&Serial);

  // --

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
  pinMode(BUTTON_SWITCH_PIN, INPUT_PULLDOWN);

  // ---

  button.init(BUTTON_SWITCH_PIN, LOW);

  ButtonConfig* buttonConfig = button.getButtonConfig();
  buttonConfig->setEventHandler(handleEvent);
  buttonConfig->setFeature(ButtonConfig::kFeatureClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureDoubleClick);
  buttonConfig->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig->setFeature(ButtonConfig::kFeatureRepeatPress);

  // ---

  scheduler.startNow();
}

void loop() { scheduler.execute(); }