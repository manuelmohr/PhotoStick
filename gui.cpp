#include "gui.hpp"
#include "GUIslice_drv.h"
#include "GUIslice_ex.h"
#include "bmp.hpp"
#include "util.hpp"

using namespace Gui;

const char *Gui::fileToLoad = nullptr;

namespace
{
gslc_tsGui     gui;
gslc_tsDriver  driver;
gslc_tsPage    pages[Gui::Page::MAX_PAGES];
gslc_tsElem    elems[Gui::Elem::MAX_ELEMS];
gslc_tsElemRef elemRefs[Gui::Elem::MAX_ELEMS];
gslc_tsFont    fonts[Gui::Font::MAX_FONTS];
gslc_tsXSlider sliderR;
gslc_tsXSlider sliderG;
gslc_tsXSlider sliderB;
Gui::Page      currentPage = Page::MAIN;
char           filenames[12][16];
int            selectedFile = -1;

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

  Serial.println("click");

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
      if (selectedFile != -1) {
        /* Deselect */
        Serial.println("deselect");
        gslc_ElemSetCol(gui, &elemRefs[Elem::PLAY1_BUTTON_FILE1 + selectedFile],
                        GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4, GSLC_COL_WHITE);
        gslc_ElemSetTxtCol(gui,
                           &elemRefs[Elem::PLAY1_BUTTON_FILE1 + selectedFile],
                           GSLC_COL_WHITE);
      }

      if (selectedFile == fileId) {
        selectedFile = -1;
      } else {
        /* Select */
        Serial.println("select");
        selectedFile = fileId;
        gslc_ElemSetCol(gui, elemRef, GSLC_COL_WHITE, GSLC_COL_WHITE,
                        GSLC_COL_BLUE_DK4);
        gslc_ElemSetTxtCol(gui,
                           &elemRefs[Elem::PLAY1_BUTTON_FILE1 + selectedFile],
                           GSLC_COL_BLUE_DK4);
      }
      break;
    }

    case Elem::PLAY1_BUTTON_BACK:
      fileToLoad  = nullptr;
      currentPage = Page::MAIN;
      break;

    case Elem::PLAY1_BUTTON_FORWARD:
      if (selectedFile != -1) {
        fileToLoad  = filenames[selectedFile];
        currentPage = Page::PLAY2;
      }
      break;
    }
    break;

  case Page::PLAY2:
    switch (id) {
    case Elem::PLAY2_BUTTON_BACK:
      currentPage = Page::PLAY1;
      break;

    case Elem::PLAY2_BUTTON_GO:
      currentPage = Page::MAIN; // TODO
      break;
    }
    break;

  case Page::CREATIVE1:
    switch (id) {
    case Elem::CREATIVE1_BUTTON_BACK:
      currentPage = Page::MAIN;
      break;

    case Elem::CREATIVE1_BUTTON_GO:
      currentPage = Page::MAIN; // TODO
      break;
    }
    break;
  }

  return false;
}

bool sliderChanged(void *pvGui, void *pvElemRef, int16_t nPos)
{
  gslc_tsGui *    gui     = (gslc_tsGui *)(pvGui);
  gslc_tsElemRef *elemRef = (gslc_tsElemRef *)(pvElemRef);
  gslc_tsElem *   elem    = elemRef->pElem;
  gslc_tsXSlider *slider  = (gslc_tsXSlider *)(elem->pXData);

  static int16_t posSliderR = 255;
  static int16_t posSliderG = 255;
  static int16_t posSliderB = 255;

  // Fetch the new RGB component from the slider
  switch (elem->nId) {
  case Elem::CREATIVE1_SLIDER_R:
    posSliderR = nPos;
    break;
  case Elem::CREATIVE1_SLIDER_G:
    posSliderG = nPos;
    break;
  case Elem::CREATIVE1_SLIDER_B:
    posSliderB = nPos;
    break;
  default:
    break;
  }

  gslc_tsColor    col      = { posSliderR, posSliderG, posSliderB };
  gslc_tsElemRef *colorbox = &elemRefs[CREATIVE1_COLORBOX];
  gslc_ElemSetCol(gui, colorbox, GSLC_COL_WHITE, col, col);

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
} // end anonymous namespace

void Gui::init()
{
  gslc_InitDebug(&glscDebugOut);

  Serial.println(F("Initializing touchscreen..."));

  if (!gslc_Init(&gui, &driver, &pages[0], Page::MAX_PAGES, &fonts[0],
                 Font::MAX_FONTS)) {
    panic(F("failed1"));
  }

  if (!gslc_FontAdd(&gui, Font::TEXT, GSLC_FONTREF_PTR, nullptr, 1)) {
    panic(F("failed2"));
  }
  if (!gslc_FontAdd(&gui, Font::TITLE, GSLC_FONTREF_PTR, nullptr, 3)) {
    panic(F("failed3"));
  }

  {
    const size_t numElems = Elem::MAIN_END - Elem::MAIN_START + 1;
    gslc_PageAdd(&gui, Page::MAIN, &elems[Elem::MAIN_START], numElems,
                 &elemRefs[Elem::MAIN_START], numElems);
  }

  {
    const size_t numElems = Elem::PLAY1_END - Elem::PLAY1_START + 1;
    gslc_PageAdd(&gui, Page::PLAY1, &elems[Elem::PLAY1_START], numElems,
                 &elemRefs[Elem::PLAY1_START], numElems);
  }

  {
    const size_t numElems = Elem::PLAY2_END - Elem::PLAY2_START + 1;
    gslc_PageAdd(&gui, Page::PLAY2, &elems[Elem::PLAY2_START], numElems,
                 &elemRefs[Elem::PLAY2_START], numElems);
  }

  {
    const size_t numElems = Elem::PLAY3_END - Elem::PLAY3_START + 1;
    gslc_PageAdd(&gui, Page::PLAY3, &elems[Elem::PLAY3_START], numElems,
                 &elemRefs[Elem::PLAY3_START], numElems);
  }

  {
    const size_t numElems = Elem::CREATIVE1_END - Elem::CREATIVE1_START + 1;
    gslc_PageAdd(&gui, Page::CREATIVE1, &elems[Elem::CREATIVE1_START], numElems,
                 &elemRefs[Elem::CREATIVE1_START], numElems);
  }

  {
    const size_t numElems = Elem::CREATIVE2_END - Elem::CREATIVE2_START + 1;
    gslc_PageAdd(&gui, Page::CREATIVE2, &elems[Elem::CREATIVE2_START], numElems,
                 &elemRefs[Elem::CREATIVE2_START], numElems);
  }

  /*
   * MAIN PAGE
   */
  gslc_SetBkgndColor(&gui, GSLC_COL_GRAY_DK2);

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

  gslc_ElemCreateBox_P(&gui, MAIN_BOX, Page::MAIN, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateBtnTxt_P(&gui, MAIN_BUTTON_PLAY, Page::MAIN, 20, 120, 100, 50,
                          "Bild", &fonts[Font::TITLE], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, MAIN_BUTTON_CREATIVE, Page::MAIN, 160, 120, 100,
                          50, "Kreativ", &fonts[Font::TITLE], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);
  Serial.println(F("okay"));

  /*
   * PLAY1 PAGE
   */
  gslc_ElemCreateBox_P(&gui, PLAY1_BOX, Page::PLAY1, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateTxt_P(&gui, PLAY1_TITLE1, Page::PLAY1, 2, 2, 320, 50, "Bild",
                       &fonts[Font::TITLE], TMP_COL1, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateTxt_P(&gui, PLAY1_TITLE2, Page::PLAY1, 0, 0, 320, 50, "Bild",
                       &fonts[Font::TITLE], TMP_COL2, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);

  gslc_ElemCreateBtnTxt_P(&gui, PLAY1_BUTTON_BACK, Page::PLAY1, 20, 20, 50, 30,
                          "Zurueck", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, PLAY1_BUTTON_FORWARD, Page::PLAY1, 260, 20, 50,
                          30, "Weiter", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);

  uint8_t i    = 0;
  File    root = SD.open("/");
  while (File entry = root.openNextFile()) {
    const char *n = entry.name();
    if (endsWith(n, ".BMP")) {
      Serial.println(entry.name());
      strcpy(filenames[i], entry.name());
      gslc_tsElemRef *btn = gslc_ElemCreateBtnTxt(
        &gui, Elem::PLAY1_BUTTON_FILE1 + i, Page::PLAY1,
        (gslc_tsRect){ 30, 70 + i * 30, 80, 20 }, filenames[i],
        sizeof(filenames[i]), Font::TEXT, &buttonClicked);
      ++i;
    }

    if (i == (Elem::PLAY1_BUTTON_FILE12 - Elem::PLAY1_BUTTON_FILE1)) {
      break;
    }
  }
  root.close();

  /*
   * PLAY2 PAGE
   */
  gslc_ElemCreateBox_P(&gui, PLAY2_BOX, Page::PLAY2, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateTxt_P(&gui, PLAY2_TITLE1, Page::PLAY2, 2, 2, 320, 50, "Konfig",
                       &fonts[Font::TITLE], TMP_COL1, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateTxt_P(&gui, PLAY2_TITLE2, Page::PLAY2, 0, 0, 320, 50, "Konfig",
                       &fonts[Font::TITLE], TMP_COL2, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateBtnTxt_P(&gui, PLAY2_BUTTON_BACK, Page::PLAY2, 20, 20, 50, 30,
                          "Zurueck", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, PLAY2_BUTTON_GO, Page::PLAY2, 160, 200, 50, 30,
                          "START", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);

  /*
   * CREATIVE1 PAGE
   */
  {
    uint16_t nSlideW = 110;
    uint16_t nSlideH = 20;

    gslc_ElemCreateBox_P(&gui, CREATIVE1_BOX, Page::CREATIVE1, 10, 50, 300, 180,
                         GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL,
                         NULL);
    gslc_ElemCreateTxt_P(&gui, CREATIVE1_TITLE1, Page::CREATIVE1, 2, 2, 320, 50,
                         "Konfig", &fonts[Font::TITLE], TMP_COL1,
                         GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                         false, false);
    gslc_ElemCreateTxt_P(&gui, CREATIVE1_TITLE2, Page::CREATIVE1, 0, 0, 320, 50,
                         "Konfig", &fonts[Font::TITLE], TMP_COL2,
                         GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                         false, false);
    gslc_ElemCreateBtnTxt_P(&gui, CREATIVE1_BUTTON_BACK, Page::CREATIVE1, 20,
                            20, 50, 30, "Zurueck", &fonts[Font::TEXT],
                            GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK,
                            GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                            false, false, &buttonClicked, nullptr);
    gslc_ElemCreateBtnTxt_P(&gui, CREATIVE1_BUTTON_GO, Page::CREATIVE1, 150,
                            200, 50, 30, "START", &fonts[Font::TEXT],
                            GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK,
                            GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                            false, false, &buttonClicked, nullptr);
    /* FIXME: Move to PROGMEM. */
    {
      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CREATIVE1_SLIDER_R, Page::CREATIVE1, &sliderR,
        (gslc_tsRect){ 20, 70, nSlideW, nSlideH }, 0, 255, 255, 10, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_RED, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_RED_DK4, 10, 5,
                               GSLC_COL_GRAY_DK2);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChanged);
    }
    {
      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CREATIVE1_SLIDER_G, Page::CREATIVE1, &sliderG,
        (gslc_tsRect){ 20, 130, nSlideW, nSlideH }, 0, 255, 255, 10, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_GREEN, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_GREEN_DK4, 10, 5,
                               GSLC_COL_GRAY_DK2);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChanged);
    }
    {
      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CREATIVE1_SLIDER_B, Page::CREATIVE1, &sliderB,
        (gslc_tsRect){ 20, 190, nSlideW, nSlideH }, 0, 255, 255, 10, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_BLUE, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_BLUE_DK4, 10, 5,
                               GSLC_COL_GRAY_DK2);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChanged);
    }
    {
      gslc_tsElemRef *colorbox =
        gslc_ElemCreateBox(&gui, CREATIVE1_COLORBOX, Page::CREATIVE1,
                           (gslc_tsRect){ 140, 70, 20, 130 });
      gslc_ElemSetCol(&gui, colorbox, GSLC_COL_WHITE, GSLC_COL_WHITE,
                      GSLC_COL_WHITE);
    }
  }

  gslc_SetPageCur(&gui, Page::MAIN);
  Serial.println(F("successful"));

  for (size_t i = 0; i < Gui::Elem::MAX_ELEMS; ++i) {
    Serial.print(i);
    Serial.print(":  ");
    Serial.println((unsigned)(void *)elemRefs[i].pElem, HEX);
  }
}

void Gui::update()
{
  if (gslc_GetPageCur(&gui) != currentPage) {
    gslc_SetPageCur(&gui, currentPage);
  }
  gslc_Update(&gui);
}

// vim: et ts=2
