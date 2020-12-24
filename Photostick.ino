#include "FastLED.h"
#include "SdFat.h"
#include "bmp.hpp"
#include "config.hpp"
#include "gui.hpp"
#include "sdios.h"
#include "util.hpp"
#include <SPI.h>

#include <limits.h>
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
  BMP::BMPFile  bmpFile;
  uint16_t      step;
  uint16_t      maxStep;
  unsigned long startMs;
  unsigned long durationMs;
  StickState    nextState;
  CRGB          animationColor;
  Animation     animation;
  uint8_t       repetitions;
  uint8_t       delayMs;
  SdFat         sd;
} stick;
}

#ifdef ENABLE_TIMING
namespace Timing
{
struct Stats
{
  unsigned long minMs;
  unsigned long maxMs;
  unsigned long sumMs;
  unsigned long num;

  Stats()
  {
    minMs = ULONG_MAX;
    maxMs = 0;
    sumMs = 0;
    num   = 0;
  }

  void update(unsigned long tookMs)
  {
    if (tookMs < minMs)
      minMs = tookMs;
    if (tookMs > maxMs)
      maxMs = tookMs;
    sumMs += tookMs;

    ++num;
  }

  double getAverage() const
  {
    return ((double)sumMs) / num;
  }

  void println() const
  {
    Serial.print(F("min="));
    Serial.print(minMs);
    Serial.print(F(" max="));
    Serial.print(maxMs);
    Serial.print(F(" avg="));
    Serial.println(getAverage());
  }
};

Stats statLoad;
Stats statShow;
}

#define TIME(statPtr, op)                   \
  do {                                      \
    const unsigned long startMs = millis(); \
    (op);                                   \
    const unsigned long stopMs = millis();  \
    (statPtr)->update(stopMs - startMs);    \
  } while (false)
#else
#define TIME(statPtr, op) (op)
#endif

// See:
// https://learn.adafruit.com/adafruit-2-8-tft-touch-shield-v2/backlight-touch-irq
void setBacklight(bool on)
{
  digitalWrite(BACKLIGHT_PIN, on ? HIGH : LOW);
}

void initSdCard()
{
  Serial.print(F("Initializing SD card..."));
  if (!stick.sd.begin(SD_CS, SD_SCK_MHZ(50))) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));
}

void setup()
{
  Serial.begin(9600);
  Serial.println(F("Photostick\n"));

  initSdCard();

  Gui::init(stick.sd);

  /* Enable backlight */
  pinMode(BACKLIGHT_PIN, OUTPUT);
  setBacklight(true);

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
    delay(1000);
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

  case ANIM_MARQUEE:
    FastLED.clear();
    leds[stick.step] = stick.animationColor;
    FastLED.show();
    break;
  }
}

uint16_t getAnimationSteps(Animation anim)
{
  switch (anim) {
  case ANIM_LIGHT:
    return 1;
  case ANIM_BLINK:
    return 2;
  case ANIM_MARQUEE:
    return NUM_LEDS;
  default:
    return 0;
  }
}

void loop()
{
  // Update logic
  switch (stick.state) {
  case StickState::GUI: {
#ifdef BATTERY_VOLTAGE_PIN
    const int  scaledBatteryVoltage = analogRead(BATTERY_VOLTAGE_PIN);
    const bool isBatteryVoltageLow =
      (scaledBatteryVoltage > 0 &&
       scaledBatteryVoltage <= BATTERY_VOLTAGE_THRESHOLD);
#else
    const bool isBatteryVoltageLow = false;
#endif

    Gui::update(isBatteryVoltageLow);
    break;
  }
  case StickState::IMAGE:
    setBacklight(false);
    TIME(&Timing::statLoad, BMP::loadRow(stick.bmpFile, stick.step, &leds[0]));
    TIME(&Timing::statShow, FastLED.show());
    ++stick.step;
    delay(stick.delayMs);
    break;

  case StickState::CREATIVE:
    setBacklight(false);
    animate();
    ++stick.step;
    delay(stick.delayMs);
    break;

  case StickState::PAUSE:
    setBacklight(false);
    break;
  }

  // Transition criteria
  switch (stick.state) {
  case StickState::GUI: {
    StickConfig cfg;
    if (Gui::readyToGo(cfg)) {
      if (cfg.fileToLoad != nullptr) {
        stick.nextState = StickState::IMAGE;
        BMP::open(stick.sd, stick.bmpFile, cfg.fileToLoad);
        stick.maxStep = stick.bmpFile.height;
        // TODO: Update this after optimization with SdFat.
        // We can do 848 pixel rows in about 11 seconds.
        // Hence, we do about 77 rows per second and each row takes about 13ms.
        // The 14ms are divided between showing on LEDs and loading from SD:
        //   Show: min=8 max=11 avg=8.94
        //   Load: min=2 max=11 avg=5.24
        //
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
        stick.maxStep        = getAnimationSteps(stick.animation);
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

      stick.bmpFile.file.close();

#ifdef ENABLE_TIMING
      Serial.print(F("Show: "));
      Timing::statShow.println();
      Timing::statShow = Timing::Stats();
      Serial.print(F("Load: "));
      Timing::statLoad.println();
      Timing::statLoad = Timing::Stats();
#endif
    }
    break;

  case StickState::PAUSE:
    if (millis() - stick.startMs >= stick.durationMs) {
      stick.startMs = millis();
      stick.state   = stick.nextState;
      setBacklight(true);
    }
    break;
  }
}

// vim: et ts=2
