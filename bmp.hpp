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

void open(SdFat &sd, BMPFile &bmpFile, const char *filename);
void loadRow(BMPFile &bmpFile, uint32_t row, CRGB *leds);
}

#endif
