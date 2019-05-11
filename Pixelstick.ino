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
  uint16_t      step;
  uint16_t      maxStep;
  unsigned long startMs;
  unsigned long durationMs;
  StickState    nextState;
  CRGB          animationColor;
  Animation     animation;
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

void animate()
{
  switch (stick.animation) {
  case ANIM_LIGHT:
    FastLED.showColor(stick.animationColor);
    delay(500);
    break;

  case ANIM_BLINK:
    if (stick.step % 2 == 0) {
      FastLED.showColor(stick.animationColor);
      delay(500);
    } else {
      FastLED.clear(true);
      delay(500);
    }
    break;
  }
}

void loop()
{
  // Update logic
  switch (stick.state) {
  case StickState::GUI:
    Gui::update();
    break;

  case StickState::IMAGE:
    bmpLoadRow(stick.bmpFile, stick.step, &leds[0]);
    FastLED.show();
    ++stick.step;
    delay(stick.delayMs);
    break;

  case StickState::CREATIVE:
    animate();
    ++stick.step;
    delay(stick.delayMs);
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
        stick.nextState = StickState::IMAGE;
        bmpOpen(stick.bmpFile, cfg.fileToLoad);
        stick.maxStep = stick.bmpFile.height;
        // We can do 382 pixel rows in about 16 seconds.
        // Hence, we do about 23 rows per second and each row takes about 45ms.
        // We consider 1 row per second as "0% speed", so we know the following:
        // - For 100% speed (i.e., speed == 10), delay = 0
        // - For   0% speed (i.e., speed ==  0), delay = 955
        // In between, we want to interpolate linearly.  Hence, we can use the
        // linear equation delay = 950 - speed * 95 to approximate this.
        stick.delayMs = 950 - cfg.speed * 95;
      } else {
        stick.nextState      = StickState::CREATIVE;
        stick.animation      = cfg.animation;
        stick.animationColor = cfg.animationColor;
        stick.maxStep        = 2; // TODO
        stick.delayMs        = 0; // TODO
      }

      stick.step        = 0;
      stick.state       = StickState::PAUSE;
      stick.startMs     = millis();
      stick.durationMs  = cfg.countdown * 1000;
      stick.repetitions = cfg.repetitions;
      FastLED.setBrightness(cfg.brightness * 10);
    }
    break;
  }

  case StickState::IMAGE:
  case StickState::CREATIVE:
    if (stick.step == stick.maxStep) {
      --stick.repetitions;
      stick.step = 0;
    }

    if (stick.repetitions == 0) {
      FastLED.clear(true);
      stick.state     = StickState::PAUSE;
      stick.nextState = StickState::GUI;
      stick.startMs   = millis();
    }
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
