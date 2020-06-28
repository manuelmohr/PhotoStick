#ifndef BMP_HPP
#define BMP_HPP

#include "FastLED.h"
#include "SdFat.h"
#include "util.hpp"
#include <stdint.h>

namespace BMP
{
struct BMPFile
{
  SdFile   file;
  boolean  flip;
  uint8_t  depth;
  uint32_t height;
  uint32_t imageOffset;
};

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
uint16_t read16(BMPFile &bmpFile);
uint32_t read24(BMPFile &bmpFile);
uint32_t read32(BMPFile &bmpFile);

void open(SdFat &sd, BMPFile &bmpFile, const char *filename);
void loadRow(BMPFile &bmpFile, uint32_t row, CRGB *leds);
}

#endif
