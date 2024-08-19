#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

Display::Display(logging::Logger *logger)
{
  _isAvailable = false;

  _display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
  _logger = logger;
}

void Display::setup()
{
  if (!_display->begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    _logger->log(
        logging::LoggerLevel::LOGGER_LEVEL_ERROR,
        "DISPLAY",
        "Error initializing the display");
    return;
  }

  _display->setRotation(2);

  _isAvailable = true;
}

bool Display::isAvailable()
{
  return _isAvailable;
}

void Display::loop()
{
  _display->display();
}

void Display::showPPO2(float sensor1ppO2, float sensor2ppO2, float sensor3ppO2)
{
  if (!isAvailable())
    return;

  _display->setTextWrap(false);
  _display->setTextColor(INVERSE);

  _display->clearDisplay();
  // ppO2
  _display->setCursor(0, 30);
  _display->setFont(&FreeMonoBold24pt7b);
  _display->print(sensor1ppO2);
  // Num
  _display->setFont(&FreeMonoBold9pt7b);
  _display->fillRect(SCREEN_WIDTH - 13, SCREEN_HEIGHT - 17, 13, 17, WHITE);
  _display->setCursor(SCREEN_WIDTH - 12, SCREEN_HEIGHT - 4);
  _display->print("1");
  _display->display();
  // delay(500);

  // _display->clearDisplay();
  // // ppO2
  // _display->setCursor(0, 30);
  // _display->setFont(&FreeMonoBold24pt7b);
  // _display->print(sensor2ppO2);
  // // Num
  // _display->setFont(&FreeMonoBold9pt7b);
  // _display->fillRect(SCREEN_WIDTH - 13, SCREEN_HEIGHT - 17, 13, 17, WHITE);
  // _display->setCursor(SCREEN_WIDTH - 12, SCREEN_HEIGHT - 4);
  // _display->print("2");
  // _display->display();
  // delay(500);

  // _display->clearDisplay();
  // // ppO2
  // _display->setCursor(0, 30);
  // _display->setFont(&FreeMonoBold24pt7b);
  // _display->print(sensor3ppO2);
  // // Num
  // _display->setFont(&FreeMonoBold9pt7b);
  // _display->fillRect(SCREEN_WIDTH - 13, SCREEN_HEIGHT - 17, 13, 17, WHITE);
  // _display->setCursor(SCREEN_WIDTH - 12, SCREEN_HEIGHT - 4);
  // _display->print("3");
  // _display->display();
  // delay(500);
}

void Display::showDisplayMessage(String message)
{
  if (!isAvailable())
    return;

  _display->clearDisplay();

  _display->setTextSize(2);
  _display->setTextColor(WHITE);
  _display->setCursor(0, 10);

  _display->println(message);
}
