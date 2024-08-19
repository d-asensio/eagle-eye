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

void Display::showSensorPPO2(float ppO2, uint8_t sensorChannel)
{
  if (!isAvailable())
    return;

  _display->setTextWrap(false);
  _display->setTextColor(INVERSE);

  _display->clearDisplay();
  
  // ppO2
  _display->setCursor(0, 30);
  _display->setFont(&FreeMonoBold24pt7b);
  _display->print(ppO2);

  // Num
  _display->setFont(&FreeMonoBold9pt7b);
  _display->fillRect(SCREEN_WIDTH - 13, SCREEN_HEIGHT - 17, 13, 17, WHITE);
  _display->setCursor(SCREEN_WIDTH - 12, SCREEN_HEIGHT - 4);
  _display->print(sensorChannel);
  _display->display();
}

void Display::showCenteredMessage(String message)
{
  if (!isAvailable())
    return;

  _display->setTextWrap(false);
  _display->setTextColor(WHITE);

  _display->clearDisplay();

  _display->setFont(&FreeMonoBold9pt7b);
  _display->setCursor(0, SCREEN_HEIGHT / 2);
  _display->print(message);
  _display->display();
}
