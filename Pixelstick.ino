#include "FastLED.h"
#include "SD.h"
#include "bmp.hpp"
#include "gui.hpp"
#include "util.hpp"

#include <string.h>

namespace
{
CRGB leds[NUM_LEDS];

enum class StickState
{
  GUI,
  IMAGE,
  CREATIVE,
  PAUSE,
};

struct
{
  StickState    state;
  BMPFile       bmpFile;
  uint32_t      row;
  unsigned long startMs;
  unsigned long durationMs;
  StickState    nextState;
} stick;
}

void initSdCard()
{
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Pixelstick\n"));

  initSdCard();

  Gui::init();
  stick.state = StickState::GUI;

  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(25);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop()
{
  // Update logic
  switch (stick.state) {
  case StickState::GUI:
    Gui::update();
    break;

  case StickState::IMAGE:
    bmpLoadRow(stick.bmpFile, stick.row, &leds[0]);
    stick.row = (stick.row + 1) % stick.bmpFile.height;
    FastLED.show();
    break;

  case StickState::CREATIVE:
    // TODO Display the chosen
    break;

  case StickState::PAUSE:
    // TODO: Turn off display:
    // https://learn.adafruit.com/2-8-tft-touch-shield/controlling-the-backlight
    break;
  }

  // Transition criteria
  switch (stick.state) {
  case StickState::GUI: {
    StickConfig cfg;
    if (Gui::readyToGo(cfg)) {
      if (cfg.fileToLoad != nullptr) {
        bmpOpen(stick.bmpFile, cfg.fileToLoad);
        stick.row       = 0;
        stick.nextState = StickState::IMAGE;
        stick.bmpFile   = BMPFile(); // TODO Reset correct?
      } else {
        // TODO: Condition for switch to CREATIVE
      }
      stick.state      = StickState::PAUSE;
      stick.startMs    = millis();
      stick.durationMs = 2000;
    }
    break;
  }
  case StickState::IMAGE:
    if (stick.row == stick.bmpFile.height) {
      stick.state      = StickState::PAUSE;
      stick.nextState  = StickState::GUI;
      stick.startMs    = millis();
      stick.durationMs = 2000;
    }
    break;

  case StickState::CREATIVE:
    // TODO
    break;

  case StickState::PAUSE:
    if (millis() - stick.startMs >= stick.durationMs) {
      stick.startMs = millis();
      stick.state   = stick.nextState;
      // TODO: Turn on display
    }
    break;
  }
}

// vim: et ts=2
