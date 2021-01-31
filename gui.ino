#include "GUIslice_drv.h"
#include "GUIslice_ex.h"
#include "SdFat.h"
#include "bmp.hpp"
#include "config.hpp"
#include "gui.hpp"
#include "i18n.hpp"
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
  PLAY1_BUTTON_PREV_FILESET,
  PLAY1_BUTTON_NEXT_FILESET,
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

  CREATIVE1_BOX,
  CREATIVE1_TITLE1,
  CREATIVE1_TITLE2,
  CREATIVE1_BUTTON_BACK,
  CREATIVE1_BUTTON_GO,
  CREATIVE1_SLIDER_R,
  CREATIVE1_SLIDER_G,
  CREATIVE1_SLIDER_B,
  CREATIVE1_COLORBOX,
  CREATIVE1_PATTERN_LIGHT_BOX,
  CREATIVE1_PATTERN_LIGHT_LABEL,
  CREATIVE1_PATTERN_BLINK_BOX,
  CREATIVE1_PATTERN_BLINK_LABEL,
  CREATIVE1_PATTERN_MARQUEE_BOX,
  CREATIVE1_PATTERN_MARQUEE_LABEL,

  CONFIG1_BOX,
  CONFIG1_TITLE1,
  CONFIG1_TITLE2,
  CONFIG1_BUTTON_BACK,
  CONFIG1_BUTTON_GO,
  CONFIG1_TEXT_BRIGHTNESS,
  CONFIG1_SLIDER_BRIGHTNESS,
  CONFIG1_INFO_BRIGHTNESS,
  CONFIG1_TEXT_SPEED,
  CONFIG1_SLIDER_SPEED,
  CONFIG1_INFO_SPEED,
  CONFIG1_TEXT_COUNTDOWN,
  CONFIG1_SLIDER_COUNTDOWN,
  CONFIG1_INFO_COUNTDOWN,
  CONFIG1_TEXT_REPETITIONS,
  CONFIG1_SLIDER_REPETITIONS,
  CONFIG1_INFO_REPETITIONS,

  MAX_ELEMS,

  MAIN_START      = MAIN_BOX,
  MAIN_END        = MAIN_BUTTON_CREATIVE,
  PLAY1_START     = PLAY1_BOX,
  PLAY1_END       = PLAY1_BUTTON_FILE10,
  CREATIVE1_START = CREATIVE1_BOX,
  CREATIVE1_END   = CREATIVE1_PATTERN_MARQUEE_LABEL,
  CONFIG1_START   = CONFIG1_BOX,
  CONFIG1_END     = CONFIG1_INFO_REPETITIONS,
};

enum Font
{
  TEXT = 0,
  TITLE,
  MAX_FONTS,
};
}

namespace
{
const uint8_t MAX_FILES           = 10;
const uint8_t NUM_FILES_COLUMNS   = 2;
const uint8_t MAX_FILENAME_LENGTH = 16;
}

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
gslc_tsXCheckbox checkboxLight;
gslc_tsXCheckbox checkboxBlink;
gslc_tsXCheckbox checkboxMarquee;
gslc_tsXSlider   sliderBrightness;
gslc_tsXSlider   sliderSpeed;
gslc_tsXSlider   sliderCountdown;
gslc_tsXSlider   sliderRepetitions;
char             filenames[MAX_FILES][MAX_FILENAME_LENGTH];
int              selectedFile   = -1;
Page             lastPage       = MAX_PAGES;
bool             isReadyToGo    = false;
size_t           numBmpFiles    = 0;
size_t           currentFileSet = 0;
StickConfig      stickConfig;
}

namespace
{
int16_t glscDebugOut(char ch)
{
  Serial.write(ch);
  return 0;
}

void setButtonSelection(gslc_tsGui *gui, gslc_tsElemRef *elemRef, bool doSelect)
{
  if (doSelect) {
    gslc_ElemSetCol(gui, elemRef, GSLC_COL_WHITE, GSLC_COL_WHITE,
                    GSLC_COL_BLUE_DK4);
    gslc_ElemSetTxtCol(gui, elemRef, GSLC_COL_BLUE_DK4);
  } else {
    /* Deselect */
    gslc_ElemSetCol(gui, elemRef, GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                    GSLC_COL_WHITE);
    gslc_ElemSetTxtCol(gui, elemRef, GSLC_COL_WHITE);
  }
}

bool endsWithIgnoreCase(const char *str, const char *e)
{
  const size_t strLen = strlen(str);
  const size_t eLen   = strlen(e);

  if (strLen < eLen) {
    return false;
  }

  const size_t off = strLen - eLen;
  return strcasecmp(str + off, e) == 0;
}

void setFilenameButtons(uint8_t numFileSet)
{
  const size_t numFileSets = (numBmpFiles + MAX_FILES - 1) / MAX_FILES;
  if (numFileSet >= numFileSets) {
    return;
  }

  SdFile root;
  if (!root.open("/")) {
    panic(F("Could not open SD root"));
  }

  SdFile entry;

  /* Skip the first numFilesToSkip .bmp files. */
  const size_t numFilesToSkip  = numFileSet * MAX_FILES;
  size_t       numSkippedFiles = 0;
  while (numSkippedFiles < numFilesToSkip && entry.openNext(&root, O_RDONLY)) {
    char name[MAX_FILENAME_LENGTH];
    entry.getName(&name[0], sizeof(name));
    if (endsWithIgnoreCase(&name[0], ".bmp")) {
      ++numSkippedFiles;
    }
    entry.close();
  }

  /* Set the button labels to the file names. */
  const size_t numFilesToShow = min(numBmpFiles - numSkippedFiles, MAX_FILES);
  size_t       numFiles       = 0;
  while (numFiles < numFilesToShow && entry.openNext(&root, O_RDONLY)) {
    char name[MAX_FILENAME_LENGTH];
    entry.getName(&name[0], sizeof(name));
    if (endsWithIgnoreCase(&name[0], ".bmp")) {
      gslc_tsElemRef *button = &elemRefs[Elem::PLAY1_BUTTON_FILE1 + numFiles];
      gslc_ElemSetTxtStr(&gui, button, name);
      gslc_ElemSetVisible(&gui, button, true);
      setButtonSelection(&gui, button, false);

      ++numFiles;
    }
    entry.close();
  }
  root.close();

  /* Make remaining buttons invisible. */
  for (size_t i = numFiles; i < MAX_FILES; ++i) {
    gslc_tsElemRef *button = &elemRefs[Elem::PLAY1_BUTTON_FILE1 + i];
    gslc_ElemSetVisible(&gui, button, false);
    setButtonSelection(&gui, button, false);
  }

  /* Handle first and last file sets to hide non-sensical buttons. */
  gslc_tsElemRef *prevSetButton = &elemRefs[Elem::PLAY1_BUTTON_PREV_FILESET];
  gslc_ElemSetVisible(&gui, prevSetButton, numFileSet != 0);
  gslc_tsElemRef *nextSetButton = &elemRefs[Elem::PLAY1_BUTTON_NEXT_FILESET];
  gslc_ElemSetVisible(&gui, nextSetButton, numFileSet + 1 != numFileSets);

  /* Reset selected file. */
  selectedFile = -1;
}

size_t getNumBmpFiles()
{
  SdFile root;
  if (!root.open("/")) {
    panic(F("Could not open SD root"));
  }

  size_t numBmpFiles = 0;
  SdFile entry;
  while (entry.openNext(&root, O_RDONLY)) {
    char name[MAX_FILENAME_LENGTH];
    entry.getName(&name[0], sizeof(name));
    if (endsWithIgnoreCase(&name[0], ".bmp")) {
      ++numBmpFiles;
    }
    entry.close();
  }
  root.close();

  return numBmpFiles;
}

void handleEventPageMain(void *pGui, int id)
{
  gslc_tsGui *gui = (gslc_tsGui *)pGui;

  switch (id) {
  case Elem::MAIN_BUTTON_PLAY:
    gslc_SetPageCur(gui, Page::PLAY1);
    break;

  case Elem::MAIN_BUTTON_CREATIVE:
    gslc_SetPageCur(gui, Page::CREATIVE1);
    break;
  }
}

void handleEventPagePlay1(void *pGui, int id, void *pElemRef)
{
  gslc_tsGui *    gui     = (gslc_tsGui *)pGui;
  gslc_tsElemRef *elemRef = (gslc_tsElemRef *)pElemRef;

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
  case Elem::PLAY1_BUTTON_FILE10: {
    const int fileId = id - Elem::PLAY1_BUTTON_FILE1;
    if (selectedFile != -1) {
      /* Deselect old */
      setButtonSelection(
        gui, &elemRefs[Elem::PLAY1_BUTTON_FILE1 + selectedFile], false);
    }

    if (selectedFile == fileId) {
      selectedFile = -1;
    } else {
      /* Select new */
      selectedFile = fileId;
      setButtonSelection(
        gui, &elemRefs[Elem::PLAY1_BUTTON_FILE1 + selectedFile], true);
    }
    break;
  }

  case Elem::PLAY1_BUTTON_BACK:
    stickConfig.fileToLoad = nullptr;
    gslc_SetPageCur(gui, Page::MAIN);
    break;

  case Elem::PLAY1_BUTTON_FORWARD:
    if (selectedFile != -1) {
      gslc_tsElemRef *button =
        &elemRefs[Elem::PLAY1_BUTTON_FILE1 + selectedFile];
      stickConfig.fileToLoad = gslc_ElemGetTxtStr(gui, button);
      lastPage               = Page::PLAY1;
      gslc_SetPageCur(gui, Page::CONFIG1);
    }
    break;

  case Elem::PLAY1_BUTTON_PREV_FILESET:
    if (currentFileSet > 0) {
      setFilenameButtons(--currentFileSet);
    }
    break;

  case Elem::PLAY1_BUTTON_NEXT_FILESET: {
    const size_t numFileSets = (numBmpFiles + MAX_FILES - 1) / MAX_FILES;
    if (currentFileSet + 1 < numFileSets) {
      setFilenameButtons(++currentFileSet);
    }
    break;
  }
  }
}

void handleEventPageCreative1(void *pGui, int id)
{
  gslc_tsGui *gui = (gslc_tsGui *)pGui;

  switch (id) {
  case Elem::CREATIVE1_BUTTON_BACK:
    gslc_SetPageCur(gui, Page::MAIN);
    break;

  case Elem::CREATIVE1_BUTTON_GO: {
    lastPage = Page::CREATIVE1;
    if (gslc_ElemXCheckboxGetState(
          gui, &elemRefs[Elem::CREATIVE1_PATTERN_LIGHT_BOX])) {
      stickConfig.animation = ANIM_LIGHT;
    } else if (gslc_ElemXCheckboxGetState(
                 gui, &elemRefs[Elem::CREATIVE1_PATTERN_BLINK_BOX])) {
      stickConfig.animation = ANIM_BLINK;
    } else if (gslc_ElemXCheckboxGetState(
                 gui, &elemRefs[Elem::CREATIVE1_PATTERN_MARQUEE_BOX])) {
      stickConfig.animation = ANIM_MARQUEE;
    }

    const uint8_t r =
      gslc_ElemXSliderGetPos(gui, &elemRefs[Elem::CREATIVE1_SLIDER_R]);
    const uint8_t g =
      gslc_ElemXSliderGetPos(gui, &elemRefs[Elem::CREATIVE1_SLIDER_G]);
    const uint8_t b =
      gslc_ElemXSliderGetPos(gui, &elemRefs[Elem::CREATIVE1_SLIDER_B]);
    stickConfig.animationColor = CRGB(r, g, b);

    gslc_SetPageCur(gui, Page::CONFIG1);
    break;
  }
  }
}

void handleEventPageConfig1(void *pGui, int id)
{
  gslc_tsGui *gui = (gslc_tsGui *)pGui;

  switch (id) {
  case Elem::CONFIG1_BUTTON_BACK:
    gslc_SetPageCur(gui, lastPage);
    break;

  case Elem::CONFIG1_BUTTON_GO:
    stickConfig.brightness =
      gslc_ElemXSliderGetPos(gui, &elemRefs[Elem::CONFIG1_SLIDER_BRIGHTNESS]);
    stickConfig.speed =
      gslc_ElemXSliderGetPos(gui, &elemRefs[Elem::CONFIG1_SLIDER_SPEED]);
    stickConfig.countdown =
      gslc_ElemXSliderGetPos(gui, &elemRefs[Elem::CONFIG1_SLIDER_COUNTDOWN]);
    stickConfig.repetitions =
      gslc_ElemXSliderGetPos(gui, &elemRefs[Elem::CONFIG1_SLIDER_REPETITIONS]);
    isReadyToGo = true;
    gslc_SetPageCur(gui, Page::MAIN);
    break;
  }
}

bool buttonClicked(void *pGui, void *pElemRef, gslc_teTouch event, int16_t x,
                   int16_t y)
{
  gslc_tsGui *    gui     = (gslc_tsGui *)pGui;
  gslc_tsElemRef *elemRef = (gslc_tsElemRef *)pElemRef;

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
  if (nPos < 0 || nPos > 255) {
    return false;
  }

  gslc_tsGui *    gui     = (gslc_tsGui *)(pvGui);
  gslc_tsElemRef *elemRef = (gslc_tsElemRef *)(pvElemRef);
  gslc_tsElem *   elem    = elemRef->pElem;
  gslc_tsXSlider *slider  = (gslc_tsXSlider *)(elem->pXData);

  static uint8_t posSliderR = 255;
  static uint8_t posSliderG = 255;
  static uint8_t posSliderB = 255;

  // Fetch the new RGB component from the slider
  switch (elem->nId) {
  case Elem::CREATIVE1_SLIDER_R:
    posSliderR = (uint8_t)nPos;
    break;
  case Elem::CREATIVE1_SLIDER_G:
    posSliderG = (uint8_t)nPos;
    break;
  case Elem::CREATIVE1_SLIDER_B:
    posSliderB = (uint8_t)nPos;
    break;
  default:
    break;
  }

  gslc_tsColor    col      = { posSliderR, posSliderG, posSliderB };
  gslc_tsElemRef *colorbox = &elemRefs[CREATIVE1_COLORBOX];
  gslc_ElemSetCol(gui, colorbox, GSLC_COL_WHITE, col, col);

  return true;
}

bool sliderChangedConfig(void *pvGui, void *pvElemRef, int16_t nPos)
{
  gslc_tsGui *    gui     = (gslc_tsGui *)(pvGui);
  gslc_tsElemRef *elemRef = (gslc_tsElemRef *)(pvElemRef);
  gslc_tsElem *   elem    = elemRef->pElem;
  gslc_tsXSlider *slider  = (gslc_tsXSlider *)(elem->pXData);

  char buf[6];
  switch (elem->nId) {
  case Elem::CONFIG1_SLIDER_BRIGHTNESS:
    snprintf(&buf[0], sizeof(buf), "%3d%%", 10 * nPos);
    gslc_ElemSetTxtStr(gui, &elemRefs[CONFIG1_INFO_BRIGHTNESS], &buf[0]);
    break;
  case Elem::CONFIG1_SLIDER_SPEED:
    snprintf(&buf[0], sizeof(buf), "%3d%%", 10 * nPos);
    gslc_ElemSetTxtStr(gui, &elemRefs[CONFIG1_INFO_SPEED], &buf[0]);
    break;
  case Elem::CONFIG1_SLIDER_COUNTDOWN:
    snprintf(&buf[0], sizeof(buf), "%1d Sek", nPos);
    gslc_ElemSetTxtStr(gui, &elemRefs[CONFIG1_INFO_COUNTDOWN], &buf[0]);
    break;
  case Elem::CONFIG1_SLIDER_REPETITIONS:
    snprintf(&buf[0], sizeof(buf), "%1d x", nPos);
    gslc_ElemSetTxtStr(gui, &elemRefs[CONFIG1_INFO_REPETITIONS], &buf[0]);
    break;
  default:
    break;
  }

  return true;
}
} // end anonymous namespace

void Gui::init(SdFat &sd)
{
  gslc_InitDebug(&glscDebugOut);

  Serial.println(F("Initializing touchscreen..."));

  if (!gslc_Init(&gui, &driver, &pages[0], Page::MAX_PAGES, &fonts[0],
                 Font::MAX_FONTS)) {
    panic(F("failed to initialize GUI"));
  }

#ifdef FLIP_DISPLAY
  if (!gslc_GuiRotate(&gui, 3)) {
    panic(F("failed to rotate GUI"));
  }
#endif

  if (!gslc_FontAdd(&gui, Font::TEXT, GSLC_FONTREF_PTR, nullptr, 1)) {
    panic(F("failed to add text font"));
  }
  if (!gslc_FontAdd(&gui, Font::TITLE, GSLC_FONTREF_PTR, nullptr, 3)) {
    panic(F("failed to add title font"));
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
  gslc_SetBkgndColor(&gui, GSLC_COL_GRAY_DK3);

#define TITLE_COL1 \
  (gslc_tsColor)   \
  {                \
    28, 70, 123    \
  }
#define TITLE_COL2 (gslc_tsColor){ 54, 188, 250 }
  gslc_ElemCreateTxt_P(&gui, MAIN_TITLE1, Page::MAIN, 2, 2, 320, 50,
                       STR_PROJECT_NAME, &fonts[Font::TITLE], TITLE_COL1,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);
  gslc_ElemCreateTxt_P(&gui, MAIN_TITLE2, Page::MAIN, 0, 0, 320, 50,
                       STR_PROJECT_NAME, &fonts[Font::TITLE], TITLE_COL2,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);

  gslc_ElemCreateBox_P(&gui, MAIN_BOX, Page::MAIN, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateBtnTxt_P(&gui, MAIN_BUTTON_PLAY, Page::MAIN, 80, 75, 160, 50,
                          STR_IMAGE, &fonts[Font::TITLE], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, MAIN_BUTTON_CREATIVE, Page::MAIN, 80, 145, 160,
                          50, STR_CREATIVE, &fonts[Font::TITLE], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);
  Serial.println(F("okay"));

  /*
   * PLAY1 PAGE
   */
  gslc_ElemCreateBox_P(&gui, PLAY1_BOX, Page::PLAY1, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateTxt_P(&gui, PLAY1_TITLE1, Page::PLAY1, 2, 2, 320, 50,
                       STR_IMAGE, &fonts[Font::TITLE], TITLE_COL1,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);
  gslc_ElemCreateTxt_P(&gui, PLAY1_TITLE2, Page::PLAY1, 0, 0, 320, 50,
                       STR_IMAGE, &fonts[Font::TITLE], TITLE_COL2,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);

  gslc_ElemCreateBtnTxt_P(&gui, PLAY1_BUTTON_BACK, Page::PLAY1, 10, 10, 50, 30,
                          STR_BACK, &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, PLAY1_BUTTON_FORWARD, Page::PLAY1, 260, 10, 50,
                          30, STR_FORWARD, &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);

  gslc_ElemCreateBtnTxt_P(&gui, PLAY1_BUTTON_PREV_FILESET, Page::PLAY1, 15, 120,
                          20, 40, "<-", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, PLAY1_BUTTON_NEXT_FILESET, Page::PLAY1, 285,
                          120, 20, 40, "->", &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);

  /* Create the buttons for file selection, but hide them for now. */
  for (uint8_t i = 0; i < MAX_FILES; ++i) {
    const uint8_t filesPerColumn = MAX_FILES / NUM_FILES_COLUMNS;
    const uint8_t row            = i % filesPerColumn;
    const uint8_t col            = i / filesPerColumn;

    filenames[i][0]     = '\0';
    gslc_tsElemRef *btn = gslc_ElemCreateBtnTxt(
      &gui, Elem::PLAY1_BUTTON_FILE1 + i, Page::PLAY1,
      (gslc_tsRect){ 50 + 120 * col, 70 + row * 30, 100, 20 }, &filenames[i][0],
      MAX_FILENAME_LENGTH, Font::TEXT, &buttonClicked);
    gslc_ElemSetVisible(&gui, btn, false);
  }

  numBmpFiles    = getNumBmpFiles();
  currentFileSet = 0;
  setFilenameButtons(currentFileSet);

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
                         STR_CREATIVE, &fonts[Font::TITLE], TITLE_COL1,
                         GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                         false, false);
    gslc_ElemCreateTxt_P(&gui, CREATIVE1_TITLE2, Page::CREATIVE1, 0, 0, 320, 50,
                         STR_CREATIVE, &fonts[Font::TITLE], TITLE_COL2,
                         GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                         false, false);
    gslc_ElemCreateBtnTxt_P(&gui, CREATIVE1_BUTTON_BACK, Page::CREATIVE1, 10,
                            10, 50, 30, STR_BACK, &fonts[Font::TEXT],
                            GSLC_COL_WHITE, GSLC_COL_BLUE_DK2,
                            GSLC_COL_BLUE_DK4, GSLC_COL_BLUE_DK2,
                            GSLC_COL_BLUE_DK1, GSLC_ALIGN_MID_MID, true, true,
                            &buttonClicked, nullptr);
    gslc_ElemCreateBtnTxt_P(&gui, CREATIVE1_BUTTON_GO, Page::CREATIVE1, 260, 10,
                            50, 30, STR_START, &fonts[Font::TEXT],
                            GSLC_COL_WHITE, GSLC_COL_BLUE_DK2,
                            GSLC_COL_BLUE_DK4, GSLC_COL_BLUE_DK2,
                            GSLC_COL_BLUE_DK1, GSLC_ALIGN_MID_MID, true, true,
                            &buttonClicked, nullptr);

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
        TITLE_COL2, true);
      gslc_ElemSetCol(&gui, checkbox, GSLC_COL_WHITE, GSLC_COL_WHITE,
                      GSLC_COL_WHITE);
      gslc_ElemSetGroup(&gui, checkbox, 1);
      gslc_ElemCreateTxt_P(&gui, CREATIVE1_PATTERN_LIGHT_LABEL, Page::CREATIVE1,
                           230, 70, 50, 20, STR_LIGHT_1_SEC, &fonts[Font::TEXT],
                           GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK,
                           GSLC_ALIGN_MID_MID, false, false);
    }
    {
      gslc_tsElemRef *checkbox = gslc_ElemXCheckboxCreate(
        &gui, CREATIVE1_PATTERN_BLINK_BOX, Page::CREATIVE1, &checkboxBlink,
        (gslc_tsRect){ 180, 100, 15, 15 }, true, GSLCX_CHECKBOX_STYLE_ROUND,
        TITLE_COL2, false);
      gslc_ElemSetCol(&gui, checkbox, GSLC_COL_WHITE, GSLC_COL_WHITE,
                      GSLC_COL_WHITE);
      gslc_ElemSetGroup(&gui, checkbox, 1);
      gslc_ElemCreateTxt_P(&gui, CREATIVE1_PATTERN_BLINK_LABEL, Page::CREATIVE1,
                           230, 100, 50, 20, STR_BLINK_1_SEC,
                           &fonts[Font::TEXT], GSLC_COL_WHITE, GSLC_COL_BLACK,
                           GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
    }
    {
      gslc_tsElemRef *checkbox = gslc_ElemXCheckboxCreate(
        &gui, CREATIVE1_PATTERN_MARQUEE_BOX, Page::CREATIVE1, &checkboxMarquee,
        (gslc_tsRect){ 180, 130, 15, 15 }, true, GSLCX_CHECKBOX_STYLE_ROUND,
        TITLE_COL2, false);
      gslc_ElemSetCol(&gui, checkbox, GSLC_COL_WHITE, GSLC_COL_WHITE,
                      GSLC_COL_WHITE);
      gslc_ElemSetGroup(&gui, checkbox, 1);
      gslc_ElemCreateTxt_P(&gui, CREATIVE1_PATTERN_MARQUEE_LABEL,
                           Page::CREATIVE1, 230, 130, 50, 20, STR_MARQUEE,
                           &fonts[Font::TEXT], GSLC_COL_WHITE, GSLC_COL_BLACK,
                           GSLC_COL_BLACK, GSLC_ALIGN_MID_MID, false, false);
    }
  }

  /*
   * CONFIG1 PAGE
   */
  gslc_ElemCreateBox_P(&gui, CONFIG1_BOX, Page::CONFIG1, 10, 50, 300, 180,
                       GSLC_COL_WHITE, GSLC_COL_BLACK, true, true, NULL, NULL);
  gslc_ElemCreateTxt_P(&gui, CONFIG1_TITLE1, Page::CONFIG1, 2, 2, 320, 50,
                       STR_CONFIG, &fonts[Font::TITLE], TITLE_COL1,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);
  gslc_ElemCreateTxt_P(&gui, CONFIG1_TITLE2, Page::CONFIG1, 0, 0, 320, 50,
                       STR_CONFIG, &fonts[Font::TITLE], TITLE_COL2,
                       GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                       false, false);
  gslc_ElemCreateBtnTxt_P(&gui, CONFIG1_BUTTON_BACK, Page::CONFIG1, 10, 10, 50,
                          30, STR_BACK, &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);
  gslc_ElemCreateBtnTxt_P(&gui, CONFIG1_BUTTON_GO, Page::CONFIG1, 260, 10, 50,
                          30, STR_START, &fonts[Font::TEXT], GSLC_COL_WHITE,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK4,
                          GSLC_COL_BLUE_DK2, GSLC_COL_BLUE_DK1,
                          GSLC_ALIGN_MID_MID, true, true, &buttonClicked,
                          nullptr);

  {
    const uint16_t slideWidth       = 130;
    const uint16_t slideHeight      = 30;
    const uint16_t tickLen          = 4;
    const uint16_t thumbControlSize = 12;
    {
      gslc_ElemCreateTxt_P(&gui, CONFIG1_TEXT_BRIGHTNESS, Page::CONFIG1, 10, 50,
                           120, 50, STR_BRIGHTNESS, &fonts[Font::TEXT],
                           GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK,
                           GSLC_ALIGN_MID_MID, false, false);

      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CONFIG1_SLIDER_BRIGHTNESS, Page::CONFIG1, &sliderBrightness,
        (gslc_tsRect){ 130, 60, slideWidth, slideHeight }, 0, 10, 2,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_WHITE, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_WHITE, 10, tickLen,
                               GSLC_COL_WHITE);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChangedConfig);

      static char text[] = " 20%";
      gslc_ElemCreateTxt(&gui, CONFIG1_INFO_BRIGHTNESS, Page::CONFIG1,
                         (gslc_tsRect){ 270, 60, 20, 30 }, &text[0],
                         sizeof(text), Font::TEXT);
    }
    {
      gslc_ElemCreateTxt_P(&gui, CONFIG1_TEXT_SPEED, Page::CONFIG1, 10, 80, 120,
                           50, STR_SPEED, &fonts[Font::TEXT], GSLC_COL_WHITE,
                           GSLC_COL_BLACK, GSLC_COL_BLACK, GSLC_ALIGN_MID_MID,
                           false, false);

      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CONFIG1_SLIDER_SPEED, Page::CONFIG1, &sliderSpeed,
        (gslc_tsRect){ 130, 90, slideWidth, slideHeight }, 0, 10, 10,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_WHITE, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_WHITE, 10, tickLen,
                               GSLC_COL_WHITE);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChangedConfig);

      static char text[] = "100%";
      gslc_ElemCreateTxt(&gui, CONFIG1_INFO_SPEED, Page::CONFIG1,
                         (gslc_tsRect){ 270, 90, 20, 30 }, &text[0],
                         sizeof(text), Font::TEXT);
    }
    {
      gslc_ElemCreateTxt_P(&gui, CONFIG1_TEXT_COUNTDOWN, Page::CONFIG1, 10, 110,
                           120, 50, STR_COUNTDOWN, &fonts[Font::TEXT],
                           GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK,
                           GSLC_ALIGN_MID_MID, false, false);

      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CONFIG1_SLIDER_COUNTDOWN, Page::CONFIG1, &sliderCountdown,
        (gslc_tsRect){ 130, 120, slideWidth, slideHeight }, 0, 5, 2,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_WHITE, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_WHITE, 6, tickLen,
                               GSLC_COL_WHITE);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChangedConfig);

      static char text[] = "2 Sek";
      gslc_ElemCreateTxt(&gui, CONFIG1_INFO_COUNTDOWN, Page::CONFIG1,
                         (gslc_tsRect){ 270, 120, 20, 30 }, &text[0],
                         sizeof(text), Font::TEXT);
    }
    {
      gslc_ElemCreateTxt_P(&gui, CONFIG1_TEXT_REPETITIONS, Page::CONFIG1, 10,
                           140, 120, 50, STR_REPETITIONS, &fonts[Font::TEXT],
                           GSLC_COL_WHITE, GSLC_COL_BLACK, GSLC_COL_BLACK,
                           GSLC_ALIGN_MID_MID, false, false);

      gslc_tsElemRef *slider = gslc_ElemXSliderCreate(
        &gui, CONFIG1_SLIDER_REPETITIONS, Page::CONFIG1, &sliderRepetitions,
        (gslc_tsRect){ 130, 150, slideWidth, slideHeight }, 1, 5, 1,
        thumbControlSize, false);
      gslc_ElemSetCol(&gui, slider, GSLC_COL_WHITE, GSLC_COL_BLACK,
                      GSLC_COL_BLACK);
      gslc_ElemXSliderSetStyle(&gui, slider, true, GSLC_COL_WHITE, 5, tickLen,
                               GSLC_COL_WHITE);
      gslc_ElemXSliderSetPosFunc(&gui, slider, &sliderChangedConfig);

      static char text[] = "1 x";
      gslc_ElemCreateTxt(&gui, CONFIG1_INFO_REPETITIONS, Page::CONFIG1,
                         (gslc_tsRect){ 270, 150, 20, 30 }, &text[0],
                         sizeof(text), Font::TEXT);
    }
  }

  /* Init */
  gslc_SetPageCur(&gui, Page::MAIN);

  Serial.println(F("successful"));
}

void Gui::update(bool enableWarning)
{
  if (enableWarning) {
    gslc_SetBkgndColor(&gui, GSLC_COL_RED_DK1);
  }

  gslc_Update(&gui);
}

bool Gui::readyToGo(StickConfig &cfg)
{
  if (!isReadyToGo) {
    return false;
  }

  cfg = stickConfig;

  // Reset
  stickConfig = StickConfig();
  isReadyToGo = false;

  return true;
}

// vim: et ts=2
