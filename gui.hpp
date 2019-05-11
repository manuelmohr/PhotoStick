#ifndef GUI_HPP
#define GUI_HPP

#include "GUIslice.h"
#include "GUIslice_drv.h"
#include "GUIslice_ex.h"

namespace Gui
{
void        init();
void        update();
const char *consumeFileToLoad();
}

#endif

// vim: et ts=2
