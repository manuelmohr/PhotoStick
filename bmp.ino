#include "FastLED.h"
#include "SdFat.h"
#include "bmp.hpp"

using namespace BMP;

// These read 16- and 32-bit values from the SD card file.
// BMP data is stored little-endian format.
uint16_t BMP::read16(BMPFile &bmpFile)
{
  SdFile & f      = bmpFile.file;
  uint16_t result = 0;
  result |= uint16_t(f.read()) << 0;
  result |= uint16_t(f.read()) << 8;
  return result;
}

uint32_t BMP::read32(BMPFile &bmpFile)
{
  SdFile & f      = bmpFile.file;
  uint32_t result = 0;
  result |= uint32_t(f.read()) << 0;
  result |= uint32_t(f.read()) << 8;
  result |= uint32_t(f.read()) << 16;
  result |= uint32_t(f.read()) << 24;
  return result;
}

void BMP::open(SdFat &sd, BMPFile &bmpFile, const char *filename)
{
  Serial.println();
  Serial.print(F("Loading image "));
  Serial.println(filename);

  if (!bmpFile.file.open(filename, O_RDONLY)) {
    panic(F("File not found"));
  }

  // Parse BMP header
  if (BMP::read16(bmpFile) != 0x4D42) {
    panic(F("Invalid header"));
  }
  Serial.print(F("File size: "));
  Serial.println(BMP::read32(bmpFile));
  (void)BMP::read32(bmpFile); // Ignore reserved word
  bmpFile.imageOffset = BMP::read32(bmpFile);
  Serial.print(F("Image Offset: "));
  Serial.println(bmpFile.imageOffset, DEC);

  // Read BMP Info header
  Serial.print(F("Header size: "));
  Serial.println(BMP::read32(bmpFile));

  if (BMP::read32(bmpFile) != BMP_WIDTH) {
    panic(F("Width must be 288"));
  }

  uint32_t height = BMP::read32(bmpFile);
  boolean  flip   = true;
  if (height < 0) {
    // If height is negative, image is in top-down order.
    // This is not common but has been observed in the wild.
    height = -height;
    flip   = false;
  }
  bmpFile.height = height;
  bmpFile.flip   = flip;

  if (BMP::read16(bmpFile) != 1) {
    panic(F("Planes must be 1"));
  }
  bmpFile.depth = BMP::read16(bmpFile);
  if (bmpFile.depth != 16 && bmpFile.depth != 24 && bmpFile.depth != 32) {
    panic(F("Depth must be 16, 24, or 32"));
  }
  const uint32_t comp = BMP::read32(bmpFile);
  if (comp != 0 && comp != 3) {
    panic(F("Must be uncompressed"));
  }

  // BMP rows are padded (if needed) to 4-byte boundary
  const uint32_t rowSize = (BMP_WIDTH * bmpFile.depth / 8 + 3U) & ~3U;

  Serial.print(F("Image size: 288"));
  Serial.print('x');
  Serial.println(bmpFile.height);
  Serial.print(F("Row size: "));
  Serial.println(rowSize);
}

void BMP::loadRow(BMPFile &bmpFile, uint32_t row, CRGB *leds)
{
  const uint32_t rowSize = (BMP_WIDTH * bmpFile.depth / 8 + 3U) & ~3U;

  uint32_t pos;
  if (bmpFile.flip) { // Normal case
    pos = bmpFile.imageOffset + (bmpFile.height - 1 - row) * rowSize;
  } else {
    pos = bmpFile.imageOffset + row * rowSize;
  }

  SdFile &file = bmpFile.file;
  if (file.curPosition() != pos) {
    file.seekSet(pos);
  }

  switch (bmpFile.depth) {
  case 16: {
    // TODO: Keep this?  It's massively slower due to the conversion.
    const int bytesPerPixel = 2;
    uint8_t * buf           = (uint8_t *)leds;
    const int numRead       = file.read(&buf[0], bytesPerPixel * NUM_LEDS);

    for (int16_t i = NUM_LEDS - 1; i >= 0; --i) {
      // BMP data is stored little-endian format.
      uint16_t c = 0;
      c |= buf[i * bytesPerPixel + 0] << 0;
      c |= buf[i * bytesPerPixel + 1] << 8;

      uint8_t r = (c & 0xF800U) >> 11;
      uint8_t g = (c & 0x07E0U) >> 5;
      uint8_t b = (c & 0x001FU) >> 0;

      r = ((uint16_t(r) * 255) / ((1U << 5) - 1));
      g = ((uint16_t(g) * 255) / ((1U << 6) - 1));
      b = ((uint16_t(b) * 255) / ((1U << 5) - 1));

      leds[i] = CRGB(r, g, b);
    }

    break;
  }
  case 24: {
    // TODO: This is basically as fast as the version below; merge them?
    const int bytesPerPixel = 3;
    uint8_t * buf           = (uint8_t *)leds;
    const int numRead       = file.read(&buf[0], bytesPerPixel * NUM_LEDS);

    for (int16_t i = NUM_LEDS - 1; i >= 0; --i) {
      // Format is RGB:       RR GG BB
      // In little endian:    0  1  2
      //                      BB GG RR
      const uint8_t r = buf[i * bytesPerPixel + 2];
      const uint8_t g = buf[i * bytesPerPixel + 1];
      const uint8_t b = buf[i * bytesPerPixel + 0];

      leds[i] = CRGB(r, g, b);
    }

    break;
  }
  case 32: {
    // In this case, unfortunately we cannot use the "leds" array as our
    // buffer as it is too small.
    // We work around this problem by using a local buffer.
    const int bytesPerPixel      = 4;
    const int pixelsPerIteration = 16; // Decrease if RAM is tight
    const int bufSize            = pixelsPerIteration * bytesPerPixel;

    int16_t pos = 0;
    do {
      uint8_t   buf[bufSize];
      const int numBytesRead = file.read(&buf[0], sizeof(buf));

      const int numPixelsRead = numBytesRead / bytesPerPixel;

      for (int i = 0; i < numPixelsRead; ++i) {
        // Format is XRGB/ARGB: AA RR GG BB
        // In little endian:    0  1  2  3
        //                      BB GG RR AA
        const uint8_t r = buf[i * bytesPerPixel + 2];
        const uint8_t g = buf[i * bytesPerPixel + 1];
        const uint8_t b = buf[i * bytesPerPixel + 0];

        leds[pos + i] = CRGB(r, g, b);
      }

      pos += numPixelsRead;
    } while (pos < NUM_LEDS);

    break;
  }
  }

#if 0
  for (int16_t i = 0; i < NUM_LEDS; ++i) {
    Serial.print(i, DEC);
    Serial.print(F(": r="));
    Serial.print(leds[i].r, HEX);
    Serial.print(F(" g="));
    Serial.print(leds[i].g, HEX);
    Serial.print(F(" b="));
    Serial.print(leds[i].b, HEX);
    Serial.println(F(""));
  }
#endif
}

// vim: et ts=2
