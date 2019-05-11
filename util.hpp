#ifndef UTIL_HPP
#define UTIL_HPP

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

struct StickConfig
{
  const char *fileToLoad;
};

#endif

// vim: et ts=2
