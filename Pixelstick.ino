#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include "SD.h"
#include "FastLED.h"

#include "arena.hpp"
#include "util.hpp"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 *tft = nullptr;

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 *ts = nullptr;

// SD card chip select
#define SD_CS 4

// How many leds in your strip?
#define NUM_LEDS 288

#define BMP_WIDTH 341

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

CRGB *leds = nullptr;

struct BMPFile
{
  File     file;
  boolean  flip;
  uint32_t height;
  uint32_t imageOffset;
};

BMPFile *bmpFile = nullptr;

#define ARENA_SIZE (1024 + sizeof(SDClass))

Arena<ARENA_SIZE> arena;

template<size_t SIZE> void *operator new(size_t size, Arena<SIZE>& a)
{
  return a.allocate(size);
}

void *operator new(size_t size, void *ptr)
{
  return ptr;
}

void operator delete(void *obj, void *alloc)
{
  return;
}

void initScreen()
{
  tft = new (arena) Adafruit_ILI9341(TFT_CS, TFT_DC);
  ts = new (arena) Adafruit_STMPE610(STMPE_CS);

  Serial.print(F("Initializing touchscreen..."));
  tft->begin();
  if (!ts->begin()) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));

  tft->fillScreen(ILI9341_BLACK);
}

void deinitScreen()
{
  tft->fillScreen(ILI9341_BLACK);
  arena.destroy(ts);
  arena.destroy(tft);
}

void initSdCard()
{
  SD = new (arena) SDClass;
  SdVolume::initCacheBuffer(arena.allocate(1024));
  Serial.print(F("Initializing SD card..."));
  if (!SD->begin(SD_CS)) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));
}

void deinitSdCard()
{
  arena.destroy(SD);
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Pixelstick\n"));

  initScreen();
  deinitScreen();
  arena.reset();

  initSdCard();
  bmpOpen("TestPad.bmp");
  for (uint16_t row = 0; row < 1; ++row) {
    bmpLoadRow(row);
    FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
    FastLED.setBrightness(1);
    FastLED.show();
  }
  bmpFile->file.close();
  deinitSdCard();
  arena.reset();
}

void loop()
{
}

void bmpOpen(const char *filename)
{
  Serial.println();
  Serial.print(F("Loading image "));
  Serial.println(filename);

  bmpFile = new (arena) BMPFile;
  bmpFile->file = SD->open(filename);
  if (!bmpFile->file) {
    panic(F("File not found"));
  }

  // Parse BMP header
  if (bmpRead16() != 0x4D42) {
    panic(F("Invalid header"));
  }
  Serial.print(F("File size: "));
  Serial.println(bmpRead32());
  (void)bmpRead32(); // Ignore reserved word
  bmpFile->imageOffset = bmpRead32();
  Serial.print(F("Image Offset: "));
  Serial.println(bmpFile->imageOffset, DEC);

  // Read BMP Info header
  Serial.print(F("Header size: "));
  Serial.println(bmpRead32());

  if (bmpRead32() != BMP_WIDTH) {
    panic(F("Width must be 341"));
  }

  uint32_t height = bmpRead32();
  boolean  flip   = true;
  if (height < 0) {
    height = -height;
    flip   = false;
  }
  bmpFile->height = height;
  bmpFile->flip   = flip;

  if (bmpRead16() != 1) {
    panic(F("Planes must be 1"));
  }
  if (bmpRead16() != 16) {
    panic(F("Depth must be 16"));
  }
  const uint32_t depth = bmpRead32();
  if (depth != 0 && depth != 3) {
    panic(F("Must be uncompressed"));
  }

  // BMP rows are padded (if needed) to 4-byte boundary
  const uint32_t rowSize = (BMP_WIDTH * 3U + 3U) & ~3U;

  // If bmpHeight is negative, image is in top-down order.
  // This is not canon but has been observed in the wild.
  Serial.print(F("Image size: 341"));
  Serial.print('x');
  Serial.println(bmpFile->height);
  Serial.print(F("Row size: "));
  Serial.println(rowSize);
}

void bmpLoadRow(uint32_t row)
{
  const uint32_t rowSize = (BMP_WIDTH * 3U + 3U) & ~3U;

  uint32_t pos;
  if (bmpFile->flip) { // Normal case
    pos = bmpFile->imageOffset + (bmpFile->height - 1 - row) * rowSize;
  } else {
    pos = bmpFile->imageOffset + row * rowSize;
  }

  File& file = bmpFile->file;
  if (file.position() != pos) {
    file.seek(pos);
  }

  uint8_t *rawData = (uint8_t*)SdVolume::getRawCacheBuffer();
  uint8_t *ledMem  = rawData + 1024 - NUM_LEDS * sizeof(CRGB);
  for (int16_t i = NUM_LEDS - 1; i >= 0; --i) {
    uint16_t c;
    c  = rawData[2 * i + 0] << 8U;
    c |= rawData[2 * i + 1] << 0U;

    const uint8_t r = (c & 0x7C00U) >> 10;
    const uint8_t g = (c & 0x03E0U) >>  5;
    const uint8_t b = (c & 0x001FU) >>  0;

    new ((void*)&ledMem[i]) CRGB(r, g, b);
  }
  leds = (CRGB*)ledMem;
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.
uint16_t bmpRead16()
{
  File& f = bmpFile->file;
  uint16_t result;
  ((uint8_t*)&result)[0] = f.read(); // LSB
  ((uint8_t*)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t bmpRead32()
{
  File& f = bmpFile->file;
  uint32_t result;
  ((uint8_t*)&result)[0] = f.read(); // LSB
  ((uint8_t*)&result)[1] = f.read();
  ((uint8_t*)&result)[2] = f.read();
  ((uint8_t*)&result)[3] = f.read(); // MSB
  return result;
}

// vim: et ts=2
