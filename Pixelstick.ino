#include "FastLED.h"
#include "SD.h"

#include "GUIslice.h"
#include "GUIslice_drv.h"
#include "GUIslice_ex.h"

#include "util.hpp"

// SD card chip select
#define SD_CS 4

// How many leds in your strip?
#define NUM_LEDS 288

// For led chips like Neopixels, which have a data line, ground, and power, you
// just need to define DATA_PIN.  For led chipsets that are SPI based (four
// wires - data, clock, ground, and power), like the LPD8806 define both
// DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

CRGB leds[NUM_LEDS];

struct PixelFile
{
  File     file;
  uint16_t columns;
};

PixelFile *pixelFile = nullptr;

enum
{
  E_PG_MAIN = 0,
  E_PG_PLAY,
  E_PG_CREATIVE,
  GUI_MAX_PAGES,
};

enum
{
  E_ELEM_BOX = 0,
};

enum
{
  E_FONT_TXT = 0,
  E_FONT_TITLE,
  GUI_MAX_FONTS,
};

#define GUI_MAX_ELEMS_RAM 1
#define GUI_MAX_ELEMS_PER_PAGE 16

gslc_tsGui *    guiGui      = nullptr;
gslc_tsDriver * guiDriver   = nullptr;
gslc_tsPage *   guiPages    = nullptr;
gslc_tsElem *   guiElem     = nullptr;
gslc_tsElemRef *guiElemRefs = nullptr;
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
  }
  return true;
}

bool GoCreative(void *pvGui, void *pvElemRef, gslc_teTouch eTouch, int16_t nX,
                int16_t nY)
{
  if (eTouch == GSLC_TOUCH_UP_IN) {
    Serial.println("Foo");
    guiPageNum = E_PG_CREATIVE;
  }
  return true;
}

void initScreen()
{
  gslc_InitDebug(&glscDebugOut);

  Serial.print(F("Initializing touchscreen..."));
  guiGui      = new gslc_tsGui();
  guiDriver   = new gslc_tsDriver();
  guiPages    = new gslc_tsPage[GUI_MAX_PAGES];
  guiElem     = new gslc_tsElem();
  guiElemRefs = new gslc_tsElemRef[GUI_MAX_ELEMS_PER_PAGE];

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
  gslc_ElemCreateTxt_P(guiGui, 98, E_PG_MAIN, 2, 2, 320, 50, "Pixelstick",
                       &guiFonts[1], TMP_COL1, GSLC_COL_BLACK, GSLC_COL_BLACK,
                       GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateTxt_P(guiGui, 99, E_PG_MAIN, 0, 0, 320, 50, "Pixelstick",
                       &guiFonts[1], TMP_COL2, GSLC_COL_BLACK, GSLC_COL_BLACK,
                       GSLC_ALIGN_MID_MID, false, false);

  // Create background box
  gslc_ElemCreateBox_P(guiGui, 200, E_PG_MAIN, 10, 50, 300, 180, GSLC_COL_WHITE,
                       GSLC_COL_BLACK, true, true, NULL, NULL);

  gslc_ElemCreateBtnTxt_P(guiGui, 201, E_PG_MAIN, 20, 120, 100, 50, "Play",
                          &guiFonts[1], GSLC_COL_WHITE, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_ALIGN_MID_MID, false, false, &GoPlay, nullptr);

  gslc_ElemCreateBtnTxt_P(guiGui, 202, E_PG_MAIN, 160, 120, 100, 50, "Creative",
                          &guiFonts[1], GSLC_COL_WHITE, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_ALIGN_MID_MID, false, false, &GoCreative,
                          nullptr);

  gslc_SetPageCur(guiGui, E_PG_MAIN);
  Serial.println(F("successful"));
}

void deinitScreen()
{
  gslc_SetBkgndColor(guiGui, GSLC_COL_BLACK);
  gslc_Update(guiGui);

  delete[] guiElemRefs;
  delete guiElem;
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

  initScreen();
  gslc_Update(guiGui);

  initSdCard();

  File root = SD.open("/");
  while (File entry = root.openNextFile()) {
    Serial.println(entry.name());
  }

  for (uint16_t i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CRGB(0xFF, 0x00, 0x00);
  }

  FastLED.setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(25);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
}

void loop()
{
  gslc_SetPageCur(guiGui, guiPageNum);
  gslc_Update(guiGui);

  FastLED.show();
  static uint16_t col = 1;
  col                 = (col + 1) % pixelFile->columns;
  //  pixelLoadColumn(col);
}

void pixelOpen(const char *filename)
{
  Serial.println();
  Serial.print(F("Loading pixels "));
  Serial.println(filename);

  pixelFile       = new PixelFile;
  pixelFile->file = SD.open(filename);
  if (!pixelFile->file) {
    panic(F("File not found"));
  }

  const uint16_t columns = pixelRead16();
  Serial.print(F("Columns: "));
  Serial.println(columns);
  pixelFile->columns = columns;
}

void pixelLoadColumn(uint16_t col)
{
  const uint32_t pos = (col + 1) * 1024;

  File &file = pixelFile->file;
  if (file.position() != pos) {
    file.seek(pos);
  }
  (void)file.read();

  // TODO
  // leds = (CRGB*)rawData;
}

uint16_t pixelRead16()
{
  File &   f = pixelFile->file;
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

// vim: et ts=2
