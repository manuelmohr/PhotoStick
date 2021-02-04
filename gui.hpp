#ifndef GUI_HPP
#define GUI_HPP

#include "GUIslice.h"
#include "GUIslice_drv.h"
#include "GUIslice_ex.h"
#include "SdFat.h"
#include "util.hpp"

enum Animation
{
  ANIM_LIGHT,
  ANIM_BLINK,
  ANIM_MARQUEE,
};

struct StickConfig
{
  const char *fileToLoad;
  Animation   animation;
  CRGB        animationColor;
  uint8_t     brightness;
  uint8_t     speed;
  uint8_t     countdown;
  uint8_t     repetitions;
};

namespace Gui
{
// Initialize GUI.  Needs SD access to enumerate BMP files.
void init(SdFat &sd);

// Update GUI, must be called periodically.  If enableWarning is true, sets
// background color to red (to warn about low battery voltage).
void update(bool enableWarning);

// Return true if user has requested to launch IMAGE or CREATIVE mode.
// In this case, cfg will be updated to carry all user-configured settings.
// Otherwise, return false.
bool readyToGo(StickConfig &cfg);
}

#endif

// vim: et ts=2
