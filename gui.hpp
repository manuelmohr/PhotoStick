#ifndef GUI_HPP
#define GUI_HPP

#include "GUIslice.h"
#include "GUIslice_drv.h"
#include "GUIslice_ex.h"
#include "util.hpp"

#define GUI_MAX_ELEMS_RAM 16
#define GUI_MAX_ELEMS_PER_PAGE 16

namespace Gui
{
enum Page
{
  MAIN = 0,
  PLAY1,
  CREATIVE1,
  MAX_PAGES,
};

enum Elem
{
  MAIN_START = 0,
  MAIN_BOX   = MAIN_START,
  MAIN_TITLE1,
  MAIN_TITLE2,
  MAIN_BUTTON_PLAY,
  MAIN_BUTTON_CREATIVE,
  PLAY1_START,
  PLAY1_BOX = PLAY1_START,
  PLAY1_TITLE1,
  PLAY1_TITLE2,
  PLAY1_BUTTON_BACK,
  PLAY1_BUTTON_FORWARD,
  PLAY1_BUTTON_FILE1,
  PLAY1_BUTTON_FILE2,
  PLAY1_BUTTON_FILE3,
  PLAY1_BUTTON_FILE4,
  PLAY1_BUTTON_FILE5,
  PLAY1_BUTTON_FILE6,
  PLAY1_BUTTON_FILE7,
  PLAY1_BUTTON_FILE8,
  PLAY1_BUTTON_FILE9,
  PLAY1_BUTTON_FILE10,
  PLAY1_BUTTON_FILE11,
  PLAY1_BUTTON_FILE12,
  PLAY2_START,
  PLAY2_BOX = PLAY2_START,
  PLAY2_TITLE,
  PLAY2_BUTTON_BACK,
  PLAY3_START,
  PLAY3_BLACK = PLAY3_START,
  MAX_ELEMS,
};

enum Font
{
  TEXT = 0,
  TITLE,
  MAX_FONTS,
};

gslc_tsGui     gui;
gslc_tsDriver  driver;
gslc_tsPage    pages[Page::MAX_PAGES];
gslc_tsElem    elems[Elem::MAX_ELEMS];
gslc_tsElemRef elemRefs[Elem::MAX_ELEMS];
gslc_tsFont    fonts[Font::MAX_FONTS];
char           filenames[12][16];

const char *fileToLoad = nullptr;

Page currentPage = Page::MAIN;

int16_t glscDebugOut(char ch)
{
  Serial.write(ch);
  return 0;
}

bool buttonClicked(void *gui, void *elemRef, gslc_teTouch event, int16_t x,
                   int16_t y)
{
  if (event != GSLC_TOUCH_UP_IN) {
    return true;
  }

  const int id = gslc_ElemGetId(gui, elemRef);
  switch (currentPage) {
  case Page::MAIN:
    switch (id) {
    case Elem::MAIN_BUTTON_PLAY:
      currentPage = Page::PLAY1;
      break;

    case Elem::MAIN_BUTTON_CREATIVE:
      currentPage = Page::CREATIVE1;
      break;
    }
    break;

  case Page::PLAY1:
    switch (id) {
    case Elem::PLAY1_BUTTON_FILE1:
    case Elem::PLAY1_BUTTON_FILE2:
    case Elem::PLAY1_BUTTON_FILE3:
    case Elem::PLAY1_BUTTON_FILE4:
    case Elem::PLAY1_BUTTON_FILE5:
    case Elem::PLAY1_BUTTON_FILE6:
    case Elem::PLAY1_BUTTON_FILE7:
    case Elem::PLAY1_BUTTON_FILE8:
    case Elem::PLAY1_BUTTON_FILE9:
    case Elem::PLAY1_BUTTON_FILE10:
    case Elem::PLAY1_BUTTON_FILE11:
    case Elem::PLAY1_BUTTON_FILE12: {
      const int fileId = id - Elem::PLAY1_BUTTON_FILE1;
      fileToLoad       = filenames[fileId];
      break;
    }
    }
    break;
  }

  return true;
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

void init()
{
  gslc_InitDebug(&glscDebugOut);

  Serial.println(F("Initializing touchscreen..."));

  if (!gslc_Init(&gui, &driver, pages, Page::MAX_PAGES, fonts,
                 Font::MAX_FONTS)) {
    panic(F("failed1"));
  }

  if (!gslc_FontAdd(&gui, Font::TEXT, GSLC_FONTREF_PTR, nullptr, 1)) {
    panic(F("failed2"));
  }
  if (!gslc_FontAdd(&gui, Font::TITLE, GSLC_FONTREF_PTR, nullptr, 3)) {
    panic(F("failed3"));
  }

  gslc_PageAdd(&gui, Page::MAIN, &elems[Elem::MAIN_START], GUI_MAX_ELEMS_RAM,
               &elemRefs[Elem::MAIN_START], GUI_MAX_ELEMS_PER_PAGE);
  gslc_PageAdd(&gui, Page::PLAY1, &elems[Elem::PLAY1_START], GUI_MAX_ELEMS_RAM,
               &elemRefs[Elem::PLAY1_START], GUI_MAX_ELEMS_PER_PAGE);

  // Background flat color
  gslc_SetBkgndColor(&gui, GSLC_COL_GRAY_DK2);

// Create Title with offset shadow
#define TMP_COL1 \
  (gslc_tsColor) \
  {              \
    32, 32, 60   \
  }
#define TMP_COL2 (gslc_tsColor){ 128, 128, 240 }
  gslc_ElemCreateTxt_P(&gui, MAIN_TITLE1, Page::MAIN, 2, 2, 320, 50,
                       "Pixelstick", &fonts[Font::TITLE], TMP_COL1,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);
  gslc_ElemCreateTxt_P(&gui, MAIN_TITLE2, Page::MAIN, 0, 0, 320, 50,
                       "Pixelstick", &fonts[Font::TITLE], TMP_COL2,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);

  // Create background box
  gslc_ElemCreateBox_P(&gui, MAIN_BOX, Page::MAIN, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateBtnTxt_P(&gui, MAIN_BUTTON_PLAY, Page::MAIN, 20, 120, 100, 50,
                          "Play", &fonts[Font::TITLE], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, MAIN_BUTTON_CREATIVE, Page::MAIN, 160, 120, 100,
                          50, "Creative", &fonts[Font::TITLE], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);

  gslc_ElemCreateBox_P(&gui, PLAY1_BOX, Page::PLAY1, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateTxt_P(&gui, PLAY1_TITLE1, Page::PLAY1, 2, 2, 320, 50, "Play",
                       &fonts[Font::TITLE], TMP_COL1, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateTxt_P(&gui, PLAY1_TITLE2, Page::PLAY1, 0, 0, 320, 50, "Play",
                       &fonts[Font::TITLE], TMP_COL2, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);

  uint8_t i    = 0;
  File    root = SD.open("/");
  while (File entry = root.openNextFile()) {
    const char *n = entry.name();
    if (endsWith(n, ".BMP")) {
      Serial.println(entry.name());
      strcpy(filenames[i], entry.name());
      gslc_ElemCreateBtnTxt(&gui, Elem::PLAY1_BUTTON_FILE1 + i, Page::PLAY1,
                            (gslc_tsRect){ 30, 70 + i * 30, 80, 20 },
                            filenames[i], sizeof(filenames[i]), Font::TEXT,
                            &buttonClicked);
      ++i;
    }

    if (i == (Elem::PLAY1_BUTTON_FILE12 - Elem::PLAY1_BUTTON_FILE1)) {
      break;
    }
  }
  root.close();

  gslc_SetPageCur(&gui, Page::MAIN);
  Serial.println(F("successful"));
}

void update()
{
  if (gslc_GetPageCur(&gui) != currentPage) {
    gslc_SetPageCur(&gui, currentPage);
  }
  gslc_Update(&gui);
}
}

#endif

// vim: et ts=2

