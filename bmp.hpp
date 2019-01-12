#ifndef BMP_HPP
#define BMP_HPP

#include "FastLED.h"
#include "SD.h"
#include "util.hpp"
#include <stdint.h>

struct BMPFile
{
  File     file;
  boolean  flip;
  uint8_t  depth;
  uint32_t height;
  uint32_t imageOffset;
};

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
uint16_t bmpRead16(BMPFile &bmpFile);
uint32_t bmpRead24(BMPFile &bmpFile);
uint32_t bmpRead32(BMPFile &bmpFile);

void bmpOpen(BMPFile &bmpFile, const char *filename);
void bmpLoadRow(BMPFile &bmpFile, uint32_t row, CRGB *leds);

#endif
