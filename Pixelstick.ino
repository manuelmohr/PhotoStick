#include "FastLED.h"
#include "SD.h"

#include "GUIslice.h"
#include "GUIslice_drv.h"
#include "GUIslice_ex.h"

#include "util.hpp"

#include <string.h>

// SD card chip select
#define SD_CS 4

// How many leds in your strip?
#define NUM_LEDS 288

#define BMP_WIDTH NUM_LEDS

// For led chips like Neopixels, which have a data line, ground, and power, you
// just need to define DATA_PIN.  For led chipsets that are SPI based (four
// wires - data, clock, ground, and power), like the LPD8806 define both
// DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

CRGB leds[NUM_LEDS];

struct BMPFile
{
  File     file;
  boolean  flip;
  uint8_t  depth;
  uint32_t height;
  uint32_t imageOffset;
};

BMPFile bmpFile;

enum
{
  E_PG_MAIN = 0,
  E_PG_PLAY,
  E_PG_CREATIVE,
  GUI_MAX_PAGES,
};

enum
{
  E_ELEM_FILE1 = 203,
  E_ELEM_FILE2,
  E_ELEM_FILE3,
  E_ELEM_FILE4,
  E_ELEM_FILE5,
  E_ELEM_MAX_FILES = E_ELEM_FILE5 - E_ELEM_FILE1 + 1,
};

static char filenames[E_ELEM_MAX_FILES][16];

enum
{
  E_FONT_TXT = 0,
  E_FONT_TITLE,
  GUI_MAX_FONTS,
};

#define GUI_MAX_ELEMS_RAM 8
#define GUI_MAX_ELEMS_PER_PAGE 32

gslc_tsGui *    guiGui       = nullptr;
gslc_tsDriver * guiDriver    = nullptr;
gslc_tsPage *   guiPages     = nullptr;
gslc_tsElem *   guiElem      = nullptr;
gslc_tsElemRef *guiElemRefs  = nullptr;
gslc_tsElem *   guiElem2     = nullptr;
gslc_tsElemRef *guiElemRefs2 = nullptr;
// Must be link-time constant as it is referenced by gslc_ElemCreateTxt_P
// and other macros, to be put into PROGMEM.
gslc_tsFont guiFonts[GUI_MAX_FONTS];

uint8_t guiPageNum = E_PG_MAIN;

static int16_t glscDebugOut(char ch)
{
  Serial.write(ch);
  return 0;
}

bool GoPlay(void *pvGui, void *pvElemRef, gslc_teTouch eTouch, int16_t nX,
            int16_t nY)
{
  if (eTouch == GSLC_TOUCH_UP_IN) {
    Serial.println("Bam");
    guiPageNum = E_PG_PLAY;
    return true;
  }
  return false;
}

bool GoCreative(void *pvGui, void *pvElemRef, gslc_teTouch eTouch, int16_t nX,
                int16_t nY)
{
  if (eTouch == GSLC_TOUCH_UP_IN) {
    Serial.println("Foo");
    guiPageNum = E_PG_CREATIVE;
    return true;
  }
  return false;
}

bool LoadFile(void *pvGui, void *pvElemRef, gslc_teTouch eTouch, int16_t nX,
              int16_t nY)
{
  if (eTouch == GSLC_TOUCH_UP_IN) {
    const int   id       = gslc_ElemGetId(pvGui, pvElemRef) - E_ELEM_FILE1;
    const char *filename = filenames[id];
    Serial.print("Load ");
    Serial.println(filename);
    bmpOpen(filename);
    return true;
  }
  return false;
}

bool endsWith(const char *str, const char *e)
{
  const size_t strLen = strlen(str);
  const size_t eLen   = strlen(e);

  if (strLen < eLen) {
    return false;
  }

  const size_t off = strLen - eLen;
  return strcmp(str + off, e) == 0;
}

void initScreen()
{
  gslc_InitDebug(&glscDebugOut);

  Serial.print(F("Initializing touchscreen..."));
  guiGui       = new gslc_tsGui();
  guiDriver    = new gslc_tsDriver();
  guiPages     = new gslc_tsPage[GUI_MAX_PAGES];
  guiElem      = new gslc_tsElem[GUI_MAX_ELEMS_RAM];
  guiElem2     = new gslc_tsElem[GUI_MAX_ELEMS_RAM];
  guiElemRefs  = new gslc_tsElemRef[GUI_MAX_ELEMS_PER_PAGE];
  guiElemRefs2 = new gslc_tsElemRef[GUI_MAX_ELEMS_PER_PAGE];

  if (!gslc_Init(guiGui, guiDriver, guiPages, GUI_MAX_PAGES, guiFonts,
                 GUI_MAX_FONTS)) {
    panic(F("failed1"));
  }

  if (!gslc_FontAdd(guiGui, E_FONT_TXT, GSLC_FONTREF_PTR, nullptr, 1)) {
    panic(F("failed2"));
  }
  if (!gslc_FontAdd(guiGui, E_FONT_TITLE, GSLC_FONTREF_PTR, nullptr, 3)) {
    panic(F("failed3"));
  }

  gslc_PageAdd(guiGui, E_PG_MAIN, guiElem, GUI_MAX_ELEMS_RAM, guiElemRefs,
               GUI_MAX_ELEMS_PER_PAGE);
  gslc_PageAdd(guiGui, E_PG_PLAY, guiElem2, GUI_MAX_ELEMS_RAM, guiElemRefs2,
               GUI_MAX_ELEMS_PER_PAGE);

  // Background flat color
  gslc_SetBkgndColor(guiGui, GSLC_COL_GRAY_DK2);

// Create Title with offset shadow
#define TMP_COL1 \
  (gslc_tsColor) \
  {              \
    32, 32, 60   \
  }
#define TMP_COL2 (gslc_tsColor){ 128, 128, 240 }
  // Note: must use title Font ID
  gslc_ElemCreateTxt_P(guiGui, 100, E_PG_MAIN, 2, 2, 320, 50, "Pixelstick",
                       &guiFonts[1], TMP_COL1, GSLC_COL_BLACK, GSLC_COL_BLACK,
                       GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateTxt_P(guiGui, 101, E_PG_MAIN, 0, 0, 320, 50, "Pixelstick",
                       &guiFonts[1], TMP_COL2, GSLC_COL_BLACK, GSLC_COL_BLACK,
                       GSLC_ALIGN_MID_MID, false, false);
  // Create background box
  gslc_ElemCreateBox_P(guiGui, 102, E_PG_MAIN, 10, 50, 300, 180, GSLC_COL_WHITE,
                       GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateBtnTxt_P(guiGui, 103, E_PG_MAIN, 20, 120, 100, 50, "Play",
                          &guiFonts[1], GSLC_COL_WHITE, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_ALIGN_MID_MID, false, false, &GoPlay, nullptr);
  gslc_ElemCreateBtnTxt_P(guiGui, 104, E_PG_MAIN, 160, 120, 100, 50, "Creative",
                          &guiFonts[1], GSLC_COL_WHITE, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_ALIGN_MID_MID, false, false, &GoCreative,
                          nullptr);

  gslc_ElemCreateBox_P(guiGui, 200, E_PG_PLAY, 10, 50, 300, 180, GSLC_COL_WHITE,
                       GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateTxt_P(guiGui, 201, E_PG_PLAY, 2, 2, 320, 50, "Play",
                       &guiFonts[1], TMP_COL1, GSLC_COL_BLACK, GSLC_COL_BLACK,
                       GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateTxt_P(guiGui, 202, E_PG_PLAY, 0, 0, 320, 50, "Play",
                       &guiFonts[1], TMP_COL2, GSLC_COL_BLACK, GSLC_COL_BLACK,
                       GSLC_ALIGN_MID_MID, false, false);

  uint8_t count = 0;
  File    root  = SD.open("/");
  while (File entry = root.openNextFile()) {
    const char *n = entry.name();
    if (endsWith(n, ".BMP")) {
      Serial.println(entry.name());
      strcpy(filenames[count], entry.name());
      gslc_ElemCreateBtnTxt(guiGui, E_ELEM_FILE1 + count, E_PG_PLAY,
                            (gslc_tsRect){ 30, 70 + count * 30, 80, 20 },
                            filenames[count], sizeof(filenames[count]),
                            E_FONT_TXT, &LoadFile);
      ++count;
    }

    if (count == E_ELEM_MAX_FILES) {
      break;
    }
  }
  root.close();

  gslc_SetPageCur(guiGui, E_PG_MAIN);
  Serial.println(F("successful"));
}

void deinitScreen()
{
  gslc_SetBkgndColor(guiGui, GSLC_COL_BLACK);
  gslc_Update(guiGui);

  delete[] guiElemRefs;
  delete[] guiElem;
  delete[] guiPages;
  delete guiDriver;
  delete guiGui;
}

void initSdCard()
{
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    panic(F("failed!"));
  }
  Serial.println(F("OK!"));
}

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Pixelstick\n"));

  initSdCard();

  File root = SD.open("/");
  while (File entry = root.openNextFile()) {
    Serial.println(entry.name());
  }
  root.close();

  initScreen();
  gslc_Update(guiGui);

  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(25);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop()
{
  if (gslc_GetPageCur(guiGui) != guiPageNum) {
    gslc_SetPageCur(guiGui, guiPageNum);
  }
  gslc_Update(guiGui);

  if (bmpFile.height != 0) {
    static uint32_t row = 0;
    bmpLoadRow(row);
    row = (row + 1) % bmpFile.height;
    FastLED.show();
  }
}

void bmpOpen(const char *filename)
{
  Serial.println();
  Serial.print(F("Loading image "));
  Serial.println(filename);

  bmpFile.file = SD.open(filename);
  if (!bmpFile.file) {
    panic(F("File not found"));
  }

  // Parse BMP header
  if (bmpRead16() != 0x4D42) {
    panic(F("Invalid header"));
  }
  Serial.print(F("File size: "));
  Serial.println(bmpRead32());
  (void)bmpRead32(); // Ignore reserved word
  bmpFile.imageOffset = bmpRead32();
  Serial.print(F("Image Offset: "));
  Serial.println(bmpFile.imageOffset, DEC);

  // Read BMP Info header
  Serial.print(F("Header size: "));
  Serial.println(bmpRead32());

  if (bmpRead32() != BMP_WIDTH) {
    panic(F("Width must be 288"));
  }

  uint32_t height = bmpRead32();
  boolean  flip   = true;
  if (height < 0) {
    height = -height;
    flip   = false;
  }
  bmpFile.height = height;
  bmpFile.flip   = flip;

  if (bmpRead16() != 1) {
    panic(F("Planes must be 1"));
  }
  bmpFile.depth = bmpRead16();
  if (bmpFile.depth != 16 && bmpFile.depth != 24 && bmpFile.depth != 32) {
    panic(F("Depth must be 16, 24, or 32"));
  }
  const uint32_t comp = bmpRead32();
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

void bmpLoadRow(uint32_t row)
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
      uint32_t c = bmpRead16();

      r = (c & 0x7C00U) >> 10;
      g = (c & 0x03E0U) >> 5;
      b = (c & 0x001FU) >> 0;

      r = (float(r) / ((1U << 5) - 1)) * 255.0f;
      g = (float(g) / ((1U << 6) - 1)) * 255.0f;
      b = (float(b) / ((1U << 5) - 1)) * 255.0f;
    } else if (bmpFile.depth == 24) {
      uint32_t c = bmpRead24();

      r = (c & 0xFF0000U) >> 16;
      g = (c & 0x00FF00U) >> 8;
      b = (c & 0x0000FFU) >> 0;
    } else {
      uint32_t c = bmpRead32();

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

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
uint16_t bmpRead16()
{
  File &   f = bmpFile.file;
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t bmpRead24()
{
  File &   f = bmpFile.file;
  uint32_t result;
  ((uint8_t *)&result)[0] = 0; // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

uint32_t bmpRead32()
{
  File &   f = bmpFile.file;
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// vim: et ts=2
