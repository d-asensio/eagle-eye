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
#include "CalibratedPPO2Read.h"

#define ATMOSPHERIC_PRESSURE_AT_SEA_LEVEL 1.01325 // SW does 1.016mb

#define BUDDY_LED_R_PIN (GPIO_NUM_18)
#define BUDDY_LED_G_PIN (GPIO_NUM_19)
#define BUDDY_LED_B_PIN (GPIO_NUM_17)

#define WET_CONTACT_PIN (GPIO_NUM_26)

#define BUTTON_SWITCH_PIN (GPIO_NUM_33)

#define BUZZER_PIN (GPIO_NUM_25)

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

// Create CalibratedPPO2Read objects with the O2VoltageSensor objects
CalibratedPPO2Read *calibratedPPO2Read1 = new CalibratedPPO2Read((O2VoltageSensor*)o2VoltageSensor1);
CalibratedPPO2Read *calibratedPPO2Read2 = new CalibratedPPO2Read((O2VoltageSensor*)o2VoltageSensor2);
CalibratedPPO2Read *calibratedPPO2Read3 = new CalibratedPPO2Read((O2VoltageSensor*)o2VoltageSensor3);

// Take calibration references

uint8_t displaySensorChannel = 1;

namespace UI {
  enum Page {calibratingPage, rotatingSensorsPage};
}

UI::Page currentUIPage = UI::rotatingSensorsPage;

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

Task endCalibrationTask(TASK_IMMEDIATE, 1, []() {
  currentUIPage = UI::rotatingSensorsPage;
  digitalWrite(BUZZER_PIN, LOW);
}, &scheduler);

Task beginCalibrationTask(TASK_IMMEDIATE, 1, []() {
  currentUIPage = UI::calibratingPage;
  digitalWrite(BUZZER_PIN, HIGH);

  calibratedPPO2Read1->takeCalibrationReference();
  calibratedPPO2Read2->takeCalibrationReference();
  calibratedPPO2Read3->takeCalibrationReference();

  endCalibrationTask.restartDelayed(500);
}, &scheduler);


void displayRotatingSensorsPage () {
  if(displaySensorChannel == 3) {
    display.showSensorPPO2(
      calibratedPPO2Read3->getPPO2(),
      displaySensorChannel
    );
    return;
  }

  if(displaySensorChannel == 2) {
    display.showSensorPPO2(
      calibratedPPO2Read2->getPPO2(),
      displaySensorChannel
    );
    return;
  }

  if(displaySensorChannel == 1) {
    display.showSensorPPO2(
      calibratedPPO2Read1->getPPO2(),
      displaySensorChannel
    );
    return;
  }
}

Task displayUITask(TASK_IMMEDIATE, TASK_FOREVER, []() {
  if(currentUIPage == UI::rotatingSensorsPage) {
    displayRotatingSensorsPage();
    return;
  }

  if(currentUIPage == UI::calibratingPage) {
    display.showCenteredMessage("Calibrating");
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
      display.showCenteredMessage("Bye");  
      delay(1500);
      display.off();  
      esp_deep_sleep_start();
    break;
  case AceButton::kEventPressed:
    logger.log(
        logging::LoggerLevel::LOGGER_LEVEL_INFO,
        "TOUCH_SENSOR",
        "Pressed");
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
    beginCalibrationTask.restart();
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

  esp_sleep_enable_ext0_wakeup(WET_CONTACT_PIN, HIGH);

  // ---

  scheduler.startNow();
}

void loop() { scheduler.execute(); }