#ifndef UTIL_HPP
#define UTIL_HPP

#include "FastLED.h"
#include "WString.h"

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
