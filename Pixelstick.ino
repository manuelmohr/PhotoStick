#include "FastLED.h"
#include "SD.h"
#include "bmp.hpp"
#include "gui.hpp"
#include "util.hpp"

#include <string.h>

CRGB    leds[NUM_LEDS];
BMPFile bmpFile;

void initSdCard()
{
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Pixelstick\n"));

  initSdCard();

  Gui::init();
  Gui::update();

  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(25);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop()
{
  Gui::update();

  if (bmpFile.height != 0) {
    static uint32_t row = 0;
    bmpLoadRow(bmpFile, row, &leds[0]);
    row = (row + 1) % bmpFile.height;
    FastLED.show();
  }
}
// vim: et ts=2
