#include <Arduino.h>
#include <Wire.h>

#include <logger.h>
#include <TaskScheduler.h>

#include <Adafruit_ADS1X15.h>

#include <AceButton.h>

#include "Display.h"

#include "ISensor.h"
#include "O2VoltageSensor.h"
#include "CalibratedPPO2Read.h"

#define BUDDY_LIGHT_LOW_PPO2_THRESHOLD (0.4)
#define BUDDY_LIGHT_HIGH_PPO2_THRESHOLD (1.6)

#define BUDDY_LED_R_PIN (GPIO_NUM_3)
#define BUDDY_LED_G_PIN (GPIO_NUM_2)
#define BUDDY_LED_B_PIN (GPIO_NUM_4)

#define WET_CONTACT_PIN (GPIO_NUM_0)
#define BUTTON_SWITCH_PIN (GPIO_NUM_1)

#define BUZZER_PIN (GPIO_NUM_10)

using namespace ace_button;

logging::Logger logger;
Scheduler scheduler;

AceButton button;

Display display(&logger);

Adafruit_ADS1115 ads;
ISensor<float> *o2VoltageSensor1 = new O2VoltageSensor(&logger, &ads, 0);
ISensor<float> *o2VoltageSensor2 = new O2VoltageSensor(&logger, &ads, 1);
ISensor<float> *o2VoltageSensor3 = new O2VoltageSensor(&logger, &ads, 2);

RTC_DATA_ATTR float calibrationReferenceVoltage1 = 52.5;
RTC_DATA_ATTR float calibrationReferenceVoltage2 = 52.5;
RTC_DATA_ATTR float calibrationReferenceVoltage3 = 52.5;

CalibratedPPO2Read *calibratedPPO2Read1 = new CalibratedPPO2Read((O2VoltageSensor*)o2VoltageSensor1, &calibrationReferenceVoltage1);
CalibratedPPO2Read *calibratedPPO2Read2 = new CalibratedPPO2Read((O2VoltageSensor*)o2VoltageSensor2, &calibrationReferenceVoltage2);
CalibratedPPO2Read *calibratedPPO2Read3 = new CalibratedPPO2Read((O2VoltageSensor*)o2VoltageSensor3, &calibrationReferenceVoltage3);

uint8_t displaySensorChannel = 1;

boolean isWet = false;

namespace LEDColor {
  const unsigned long Black = 0x000000;
  const unsigned long Red = 0xFF0000;
  const unsigned long Green = 0x00FF00;
  const unsigned long Orange = 0xFFA500;

  void displayColor(unsigned long color) {
    int red = (color >> 16) & 0xFF;  // Extract the red component
    int green = (color >> 8) & 0xFF;  // Extract the green component
    int blue = color & 0xFF;  // Extract the blue component

    analogWrite(BUDDY_LED_R_PIN, red);
    analogWrite(BUDDY_LED_G_PIN, green);
    analogWrite(BUDDY_LED_B_PIN, blue);
  }  
}

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
  isWet = digitalRead(WET_CONTACT_PIN) == HIGH;
}, &scheduler, true);

Task ppO2Alert(TASK_IMMEDIATE, TASK_FOREVER, []() {
  if (!isWet) {
    LEDColor::displayColor(LEDColor::Black);
    return;
  }

  float ppo2_1 = calibratedPPO2Read1->getPPO2();
  float ppo2_2 = calibratedPPO2Read2->getPPO2();
  float ppo2_3 = calibratedPPO2Read3->getPPO2();

  if (ppo2_1 < BUDDY_LIGHT_LOW_PPO2_THRESHOLD || ppo2_2 < BUDDY_LIGHT_LOW_PPO2_THRESHOLD || ppo2_3 < BUDDY_LIGHT_LOW_PPO2_THRESHOLD) {
    LEDColor::displayColor(LEDColor::Red);
    return;
  }
  
  if (ppo2_1 > BUDDY_LIGHT_HIGH_PPO2_THRESHOLD || ppo2_2 > BUDDY_LIGHT_HIGH_PPO2_THRESHOLD || ppo2_3 > BUDDY_LIGHT_HIGH_PPO2_THRESHOLD) {
    LEDColor::displayColor(LEDColor::Red);
    return;
  }

  LEDColor::displayColor(LEDColor::Green);
}, &scheduler, true);

void calibrationNotPossible() {
  display.showCenteredMessage("Err. In water");  
  delay(1500);
}

void calibrate() {
  boolean isWet = digitalRead(WET_CONTACT_PIN) == HIGH;
  if (isWet) {
    calibrationNotPossible();
    return;
  }
  digitalWrite(BUZZER_PIN, HIGH);
  display.showCenteredMessage("Calibrating");

  calibratedPPO2Read1->takeCalibrationReference();
  calibratedPPO2Read2->takeCalibrationReference();
  calibratedPPO2Read3->takeCalibrationReference();

  delay(1000);
  
  digitalWrite(BUZZER_PIN, LOW);
}

void turnOff() {
  boolean isWet = digitalRead(WET_CONTACT_PIN) == HIGH;
  if (isWet) {
    calibrationNotPossible();
    return;
  }

  // Kepping LED off during deep sleep
  LEDColor::displayColor(LEDColor::Black);
  gpio_hold_en(BUDDY_LED_R_PIN);
  gpio_hold_en(BUDDY_LED_G_PIN);
  gpio_hold_en(BUDDY_LED_B_PIN);
  gpio_deep_sleep_hold_en();

  display.showCenteredMessage("Bye");  
  delay(1500);
  display.off();  
  esp_deep_sleep_start();
}


void displayRotatingSensorsPage () {
  if(displaySensorChannel == 3) {
    display.showSensorPPO2(
      calibratedPPO2Read3->getPPO2(),
      displaySensorChannel,
      isWet
    );
    return;
  }

  if(displaySensorChannel == 2) {
    display.showSensorPPO2(
      calibratedPPO2Read2->getPPO2(),
      displaySensorChannel,
      isWet
    );
    return;
  }

  if(displaySensorChannel == 1) {
    display.showSensorPPO2(
      calibratedPPO2Read1->getPPO2(),
      displaySensorChannel,
      isWet
    );
    return;
  }
}

Task displayUITask(TASK_IMMEDIATE, TASK_FOREVER, []() {
  if(currentUIPage == UI::rotatingSensorsPage) {
    displayRotatingSensorsPage();
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
    turnOff();
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
    calibrate();
    break;
  }
}

void setup()
{
  #if CONFIG_SERIAL_LOG == 1
  Serial.begin(115200);
  logger.setSerial(&Serial);
  #endif

  // --

  display.setup();

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

  esp_deep_sleep_enable_gpio_wakeup((1 << BUTTON_SWITCH_PIN) | (1 << WET_CONTACT_PIN), ESP_GPIO_WAKEUP_GPIO_HIGH);

  // Disable hold on LED after deep sleep, to be able to control it again
  gpio_hold_dis(BUDDY_LED_R_PIN);
  gpio_hold_dis(BUDDY_LED_G_PIN);
  gpio_hold_dis(BUDDY_LED_B_PIN);

  // ---

  scheduler.startNow();
}

void loop() { 
  scheduler.execute();
}