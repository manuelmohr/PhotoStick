#ifndef UTIL_HPP
#define UTIL_HPP

#include "FastLED.h"
#include "WString.h"

// For led chips like Neopixels, which have a data line, ground, and power, you
// just need to define DATA_PIN.  For led chipsets that are SPI based (four
// wires - data, clock, ground, and power), like the LPD8806 define both
// DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

// SD card chip select
#define SD_CS 4

// How many leds in your strip?
#define NUM_LEDS 288

#define BMP_WIDTH NUM_LEDS

__attribute__((noreturn)) void panic(const __FlashStringHelper *pgstr);

enum Animation
{
  ANIM_LIGHT,
  ANIM_BLINK,
  ANIM_MARQUEE,
};

struct StickConfig
{
  const char *fileToLoad;
  Animation   animation;
  CRGB        animationColor;
  uint8_t     brightness;
  uint8_t     speed;
  uint8_t     countdown;
  uint8_t     repetitions;
};

#endif

// vim: et ts=2
