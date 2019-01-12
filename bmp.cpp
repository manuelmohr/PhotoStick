#include "bmp.hpp"
#include "FastLED.h"
#include "SD.h"

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
uint16_t bmpRead16(BMPFile &bmpFile)
{
  File &   f = bmpFile.file;
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t bmpRead24(BMPFile &bmpFile)
{
  File &   f = bmpFile.file;
  uint32_t result;
  ((uint8_t *)&result)[0] = 0; // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

uint32_t bmpRead32(BMPFile &bmpFile)
{
  File &   f = bmpFile.file;
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void bmpOpen(BMPFile &bmpFile, const char *filename)
{
  Serial.println();
  Serial.print(F("Loading image "));
  Serial.println(filename);

  bmpFile.file = SD.open(filename);
  if (!bmpFile.file) {
    panic(F("File not found"));
  }

  // Parse BMP header
  if (bmpRead16(bmpFile) != 0x4D42) {
    panic(F("Invalid header"));
  }
  Serial.print(F("File size: "));
  Serial.println(bmpRead32(bmpFile));
  (void)bmpRead32(bmpFile); // Ignore reserved word
  bmpFile.imageOffset = bmpRead32(bmpFile);
  Serial.print(F("Image Offset: "));
  Serial.println(bmpFile.imageOffset, DEC);

  // Read BMP Info header
  Serial.print(F("Header size: "));
  Serial.println(bmpRead32(bmpFile));

  if (bmpRead32(bmpFile) != BMP_WIDTH) {
    panic(F("Width must be 288"));
  }

  uint32_t height = bmpRead32(bmpFile);
  boolean  flip   = true;
  if (height < 0) {
    height = -height;
    flip   = false;
  }
  bmpFile.height = height;
  bmpFile.flip   = flip;

  if (bmpRead16(bmpFile) != 1) {
    panic(F("Planes must be 1"));
  }
  bmpFile.depth = bmpRead16(bmpFile);
  if (bmpFile.depth != 16 && bmpFile.depth != 24 && bmpFile.depth != 32) {
    panic(F("Depth must be 16, 24, or 32"));
  }
  const uint32_t comp = bmpRead32(bmpFile);
  if (comp != 0 && comp != 3) {
    panic(F("Must be uncompressed"));
  }

  // BMP rows are padded (if needed) to 4-byte boundary
  const uint32_t rowSize = (BMP_WIDTH * bmpFile.depth / 8 + 3U) & ~3U;

  // If bmpHeight is negative, image is in top-down order.
  // This is not canon but has been observed in the wild.
  Serial.print(F("Image size: 288"));
  Serial.print('x');
  Serial.println(bmpFile.height);
  Serial.print(F("Row size: "));
  Serial.println(rowSize);
}

void bmpLoadRow(BMPFile &bmpFile, uint32_t row, CRGB *leds)
{
  const uint32_t rowSize = (BMP_WIDTH * bmpFile.depth / 8 + 3U) & ~3U;

  uint32_t pos;
  if (bmpFile.flip) { // Normal case
    pos = bmpFile.imageOffset + (bmpFile.height - 1 - row) * rowSize;
  } else {
    pos = bmpFile.imageOffset + row * rowSize;
  }

  File &file = bmpFile.file;
  if (file.position() != pos) {
    file.seek(pos);
  }

#if 0
  Serial.print("pos: ");
  Serial.println(pos, DEC);
#endif
  for (int16_t i = 0; i < NUM_LEDS; ++i) {
    uint8_t r, g, b;

    if (bmpFile.depth == 16) {
      uint32_t c = bmpRead16(bmpFile);

      r = (c & 0x7C00U) >> 10;
      g = (c & 0x03E0U) >> 5;
      b = (c & 0x001FU) >> 0;

      r = (float(r) / ((1U << 5) - 1)) * 255.0f;
      g = (float(g) / ((1U << 6) - 1)) * 255.0f;
      b = (float(b) / ((1U << 5) - 1)) * 255.0f;
    } else if (bmpFile.depth == 24) {
      uint32_t c = bmpRead24(bmpFile);

      r = (c & 0xFF0000U) >> 16;
      g = (c & 0x00FF00U) >> 8;
      b = (c & 0x0000FFU) >> 0;
    } else {
      uint32_t c = bmpRead32(bmpFile);

      r = (c & 0xFF000000U) >> 24;
      g = (c & 0x00FF0000U) >> 16;
      b = (c & 0x0000FF00U) >> 8;
    }

    leds[i] = CRGB(r, g, b);

#if 0
    Serial.print(i, DEC);
    Serial.print(": r=");
    Serial.print(leds[i].r, HEX);
    Serial.print(" g=");
    Serial.print(leds[i].g, HEX);
    Serial.print(" b=");
    Serial.print(leds[i].b, HEX);
    Serial.println("");
#endif
  }
}
