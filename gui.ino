#include "GUIslice_drv.h"
#include "GUIslice_ex.h"
#include "bmp.hpp"
#include "gui.hpp"
#include "util.hpp"

using namespace Gui;

namespace
{
enum Page
{
  MAIN = 0,
  PLAY1,
  CREATIVE1,
  CONFIG1,
  MAX_PAGES,
};

enum Elem
{
  MAIN_BOX,
  MAIN_TITLE1,
  MAIN_TITLE2,
  MAIN_BUTTON_PLAY,
  MAIN_BUTTON_CREATIVE,

  PLAY1_BOX,
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

  CREATIVE1_BOX,
  CREATIVE1_TITLE1,
  CREATIVE1_TITLE2,
  CREATIVE1_BUTTON_BACK,
  CREATIVE1_BUTTON_GO,
  CREATIVE1_SLIDER_R,
  CREATIVE1_SLIDER_G,
  CREATIVE1_SLIDER_B,
  CREATIVE1_SLIDER_TIME,
  CREATIVE1_COLORBOX,
  CREATIVE1_PATTERN_LIGHT_BOX,
  CREATIVE1_PATTERN_LIGHT_LABEL,
  CREATIVE1_PATTERN_BLINK_BOX,
  CREATIVE1_PATTERN_BLINK_LABEL,

  CONFIG1_BOX,
  CONFIG1_TITLE1,
  CONFIG1_TITLE2,
  CONFIG1_BUTTON_BACK,
  CONFIG1_BUTTON_GO,

  MAX_ELEMS,

  MAIN_START      = MAIN_BOX,
  MAIN_END        = MAIN_BUTTON_CREATIVE,
  PLAY1_START     = PLAY1_BOX,
  PLAY1_END       = PLAY1_BUTTON_FILE12,
  CREATIVE1_START = CREATIVE1_BOX,
  CREATIVE1_END   = CREATIVE1_PATTERN_BLINK_LABEL,
  CONFIG1_START   = CONFIG1_BOX,
  CONFIG1_END     = CONFIG1_BUTTON_GO,
};

enum Font
{
  TEXT = 0,
  TITLE,
  MAX_FONTS,
};
} // end anonymous namespace enums

namespace
{
gslc_tsGui       gui;
gslc_tsDriver    driver;
gslc_tsPage      pages[Page::MAX_PAGES];
gslc_tsElem      elems[Elem::MAX_ELEMS];
gslc_tsElemRef   elemRefs[Elem::MAX_ELEMS];
gslc_tsFont      fonts[Font::MAX_FONTS];
gslc_tsXSlider   sliderR;
gslc_tsXSlider   sliderG;
gslc_tsXSlider   sliderB;
gslc_tsXSlider   sliderTime;
gslc_tsXCheckbox checkboxLight;
gslc_tsXCheckbox checkboxBlink;
char             filenames[12][16];
int              selectedFile = -1;
const char *     fileToLoad   = nullptr;
Page             lastPage     = MAX_PAGES;
bool             isReadyToGo  = false;
} // end anonymous namespace variables

namespace
{
int16_t glscDebugOut(char ch)
{
  Serial.write(ch);
  return 0;
}

void handleEventPageMain(void *gui, int id)
{
  switch (id) {
  case Elem::MAIN_BUTTON_PLAY:
    gslc_SetPageCur(gui, Page::PLAY1);
    break;

  case Elem::MAIN_BUTTON_CREATIVE:
    gslc_SetPageCur(gui, Page::CREATIVE1);
    break;
  }
}

void handleEventPagePlay1(void *gui, int id, void *elemRef)
{
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
    fileToLoad = nullptr;
    gslc_SetPageCur(gui, Page::MAIN);
    break;

  case Elem::PLAY1_BUTTON_FORWARD:
    if (selectedFile != -1) {
      fileToLoad = filenames[selectedFile];
      lastPage   = Page::PLAY1;
      gslc_SetPageCur(gui, Page::CONFIG1);
    }
    break;
  }
}

void handleEventPageCreative1(void *gui, int id)
{
  switch (id) {
  case Elem::CREATIVE1_BUTTON_BACK:
    gslc_SetPageCur(gui, Page::MAIN);
    break;

  case Elem::CREATIVE1_BUTTON_GO:
    lastPage = Page::CREATIVE1;
    gslc_SetPageCur(gui, Page::CONFIG1);
    break;
  }
}

void handleEventPageConfig1(void *gui, int id)
{
  switch (id) {
  case Elem::CONFIG1_BUTTON_BACK:
    gslc_SetPageCur(gui, lastPage);
    break;

  case Elem::CONFIG1_BUTTON_GO:
    gslc_SetPageCur(gui, Page::MAIN); // TODO
    break;
  }
}

bool buttonClicked(void *gui, void *elemRef, gslc_teTouch event, int16_t x,
                   int16_t y)
{
  if (event != GSLC_TOUCH_UP_IN) {
    return false;
  }

  const int id = gslc_ElemGetId(gui, elemRef);
  switch (gslc_GetPageCur(gui)) {
  case Page::MAIN:
    handleEventPageMain(gui, id);
    break;

  case Page::PLAY1:
    handleEventPagePlay1(gui, id, elemRef);
    break;

  case Page::CREATIVE1:
    handleEventPageCreative1(gui, id);
    break;

  case Page::CONFIG1:
    handleEventPageConfig1(gui, id);
    break;
  }

  return true;
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
  // TODO Add brightness slider
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
    const size_t numElems = Elem::CREATIVE1_END - Elem::CREATIVE1_START + 1;
    gslc_PageAdd(&gui, Page::CREATIVE1, &elems[Elem::CREATIVE1_START], numElems,
                 &elemRefs[Elem::CREATIVE1_START], numElems);
  }

  {
    const size_t numElems = Elem::CONFIG1_END - Elem::CONFIG1_START + 1;
    gslc_PageAdd(&gui, Page::CONFIG1, &elems[Elem::CONFIG1_START], numElems,
                 &elemRefs[Elem::CONFIG1_START], numElems);
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
   * CREATIVE1 PAGE
   */
  {
    const uint16_t slideWidth  = 110;
    const uint16_t slideHeight = 30;

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
    gslc_ElemCreateBtnTxt_P(&gui, CREATIVE1_BUTTON_GO, Page::CREATIVE1, 200,
                            200, 50, 30, "START", &fonts[Font::TEXT],
                            GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK,
                            GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                            false, false, &buttonClicked, nullptr);

    /* FIXME: Move to PROGMEM. */
    const uint16_t numTicks         = 0;
    const uint16_t tickLen          = 0;
    const uint16_t thumbControlSize = 12;
    {
      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CREATIVE1_SLIDER_R, Page::CREATIVE1, &sliderR,
        (gslc_tsRect){ 20, 70, slideWidth, slideHeight }, 0, 255, 255,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_RED, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_RED_DK4, numTicks,
                               tickLen, GSLC_COL_GRAY_DK2);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChanged);
    }
    {
      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CREATIVE1_SLIDER_G, Page::CREATIVE1, &sliderG,
        (gslc_tsRect){ 20, 110, slideWidth, slideHeight }, 0, 255, 255,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_GREEN, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_GREEN_DK4, numTicks,
                               tickLen, GSLC_COL_GRAY_DK2);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChanged);
    }
    {
      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CREATIVE1_SLIDER_B, Page::CREATIVE1, &sliderB,
        (gslc_tsRect){ 20, 150, slideWidth, slideHeight }, 0, 255, 255,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_BLUE, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_BLUE_DK4, numTicks,
                               tickLen, GSLC_COL_GRAY_DK2);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChanged);
    }
    {
      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CREATIVE1_SLIDER_TIME, Page::CREATIVE1, &sliderTime,
        (gslc_tsRect){ 20, 190, slideWidth, slideHeight }, 1, 10, 2,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_WHITE, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_WHITE, numTicks,
                               tickLen, GSLC_COL_GRAY_DK2);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChanged);
    }
    {
      gslc_tsElemRef *colorbox =
        gslc_ElemCreateBox(&gui, CREATIVE1_COLORBOX, Page::CREATIVE1,
                           (gslc_tsRect){ 140, 70, 20, 140 });
      gslc_ElemSetCol(&gui, colorbox, GSLC_COL_WHITE, GSLC_COL_WHITE,
                      GSLC_COL_WHITE);
    }
    {
      gslc_tsElemRef *checkbox = gslc_ElemXCheckboxCreate(
        &gui, CREATIVE1_PATTERN_LIGHT_BOX, Page::CREATIVE1, &checkboxLight,
        (gslc_tsRect){ 180, 70, 15, 15 }, true, GSLCX_CHECKBOX_STYLE_ROUND,
        TMP_COL2, true);
      gslc_ElemSetCol(&gui, checkbox, GSLC_COL_WHITE, GSLC_COL_WHITE,
                      GSLC_COL_WHITE);
      gslc_ElemSetGroup(&gui, checkbox, 1);
      gslc_ElemCreateTxt_P(&gui, CREATIVE1_PATTERN_LIGHT_LABEL, Page::CREATIVE1,
                           200, 70, 50, 20, "Leuchten", &fonts[Font::TEXT],
                           TMP_COL2, GSLC_COL_BLACK, GSLC_COL_BLACK,
                           GSLC_ALIGN_MID_MID, false, false);
    }
    {
      gslc_tsElemRef *checkbox = gslc_ElemXCheckboxCreate(
        &gui, CREATIVE1_PATTERN_BLINK_BOX, Page::CREATIVE1, &checkboxBlink,
        (gslc_tsRect){ 180, 100, 15, 15 }, true, GSLCX_CHECKBOX_STYLE_ROUND,
        TMP_COL2, false);
      gslc_ElemSetCol(&gui, checkbox, GSLC_COL_WHITE, GSLC_COL_WHITE,
                      GSLC_COL_WHITE);
      gslc_ElemSetGroup(&gui, checkbox, 1);
      gslc_ElemCreateTxt_P(&gui, CREATIVE1_PATTERN_BLINK_LABEL, Page::CREATIVE1,
                           200, 100, 50, 20, "Blinken", &fonts[Font::TEXT],
                           TMP_COL2, GSLC_COL_BLACK, GSLC_COL_BLACK,
                           GSLC_ALIGN_MID_MID, false, false);
    }
  }

  /*
   * CONFIG1 PAGE
   */
  gslc_ElemCreateBox_P(&gui, CONFIG1_BOX, Page::CONFIG1, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateTxt_P(&gui, CONFIG1_TITLE1, Page::CONFIG1, 2, 2, 320, 50,
                       "Konfig", &fonts[Font::TITLE], TMP_COL1, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateTxt_P(&gui, CONFIG1_TITLE2, Page::CONFIG1, 0, 0, 320, 50,
                       "Konfig", &fonts[Font::TITLE], TMP_COL2, GSLC_COL_BLACK,
                       GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
  gslc_ElemCreateBtnTxt_P(&gui, CONFIG1_BUTTON_BACK, Page::CONFIG1, 20, 20, 50,
                          30, "Zurueck", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, CONFIG1_BUTTON_GO, Page::CONFIG1, 160, 200, 50,
                          30, "START", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_COL_BLACK,
                          GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false,
                          &buttonClicked, nullptr);

  gslc_SetPageCur(&gui, Page::MAIN);
  Serial.println(F("successful"));
}

void Gui::update()
{
  gslc_Update(&gui);
}

bool Gui::readyToGo(StickConfig &cfg)
{
  if (!isReadyToGo) {
    return false;
  }

  cfg.fileToLoad = fileToLoad;
  return true;
}

// vim: et ts=2
