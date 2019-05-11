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
  uint8_t       repetitions;
  uint8_t       delayMs;
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
    ++stick.row;
    FastLED.show();
    delay(stick.delayMs);
    break;

  case StickState::CREATIVE:
    // TODO Display the chosen animation
    break;

  case StickState::PAUSE:
    Gui::setBacklight(false);
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
      } else {
        // TODO: Condition for switch to CREATIVE
      }
      stick.state       = StickState::PAUSE;
      stick.startMs     = millis();
      stick.durationMs  = cfg.countdown * 1000;
      stick.repetitions = cfg.repetitions;
      FastLED.setBrightness(cfg.brightness * 10);
      // We can do 382 pixel rows in about 16 seconds.
      // Hence, we do about 23 rows per second and each row takes about 45ms.
      // We consider 1 row per second as "0% speed", so we know the following:
      // - For 100% speed (i.e., speed == 10), delay = 0
      // - For   0% speed (i.e., speed ==  0), delay = 955
      // In between, we want to interpolate linearly.  Hence, we can use the
      // linear equation delay = 950 - speed * 95 to approximate this.
      stick.delayMs = 950 - cfg.speed * 95;
    }
    break;
  }
  case StickState::IMAGE:
    if (stick.row == stick.bmpFile.height) {
      --stick.repetitions;
      stick.row = 0;
    }

    if (stick.repetitions == 0) {
      FastLED.clear();
      FastLED.show();
      stick.state     = StickState::PAUSE;
      stick.nextState = StickState::GUI;
      stick.startMs   = millis();
    }
    break;

  case StickState::CREATIVE:
    // TODO
    break;

  case StickState::PAUSE:
    if (millis() - stick.startMs >= stick.durationMs) {
      stick.startMs = millis();
      stick.state   = stick.nextState;
      Gui::setBacklight(true);
    }
    break;
  }
}

// vim: et ts=2
