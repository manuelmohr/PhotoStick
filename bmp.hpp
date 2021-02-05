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

// Open given file as BMP file and save header information into struct
void open(SdFat &sd, BMPFile &bmpFile, const char *filename);

// Load pixel colors of row (counted starting from 0) into CRGB array
void loadRow(BMPFile &bmpFile, uint32_t row, CRGB *leds);
}

#endif
