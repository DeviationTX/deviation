/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#define OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/tx.h"
#include "config/model.h"
#include "autodimmer.h"

#if LCD_WIDTH == 66
//devof12e
enum {
  LARGE_SEL_X_OFFSET = 32,
  MED_SEL_X_OFFSET   = 32,
  SMALL_SEL_X_OFFSET = 32,
  TITLE_X_OFFSET     = 4,
  TITLE_WIDTH        = LCD_WIDTH - 4,
  LABEL_X_OFFSET     = 2,
  LABEL_WIDTH        = 30,
  TEXTSEL_X_WIDTH    = 24,
  CALIB_Y            = 0,
};

#else
//devof7
enum {
  LARGE_SEL_X_OFFSET = 14,
  MED_SEL_X_OFFSET   = 14,
  SMALL_SEL_X_OFFSET = 14,
  TITLE_X_OFFSET     = 2,
  TITLE_WIDTH        = LCD_WIDTH - 2,
  LABEL_X_OFFSET     = 0,
  LABEL_WIDTH        = 13,
  TEXTSEL_X_WIDTH    = LCD_WIDTH - 14 - 2,
};
#endif
#include "../128x64x1/tx_configure.c"
