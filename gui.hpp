#ifndef GUI_HPP
#define GUI_HPP

#include "GUIslice.h"
#include "GUIslice_drv.h"
#include "GUIslice_ex.h"

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

extern gslc_tsGui     gui;
extern gslc_tsDriver  driver;
extern gslc_tsPage    pages[Page::MAX_PAGES];
extern gslc_tsElem    elems[Elem::MAX_ELEMS];
extern gslc_tsElemRef elemRefs[Elem::MAX_ELEMS];
extern gslc_tsFont    fonts[Font::MAX_FONTS];
extern char           filenames[12][16];
extern const char *   fileToLoad;
extern Page           currentPage;

void init();
void update();
}

#endif

// vim: et ts=2
