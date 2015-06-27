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

#include "common.h"
#include "pages.h"
#include "gui/gui.h"

#define OVERRIDE_PLACEMENT
#define X_OFFSET (ITEM_SPACE*2 - 2)
enum {
    BUTTON_X      = X_OFFSET,
    BUTTON_WIDTH  = 6 * ITEM_SPACE,
    TEXTSEL_X     = X_OFFSET + 7 * ITEM_SPACE,
    TEXTSEL_WIDTH = 7 * ITEM_SPACE,
    LABEL_X       = X_OFFSET + 16 * ITEM_SPACE,
    LABEL_WIDTH   = 8 * ITEM_SPACE,
    STEP_X        = X_OFFSET + 7 * ITEM_SPACE,
    STEP_Y        = 0,
    STEP_WIDTH    = 8,
    TRIMPOS_X     = X_OFFSET + 16 * ITEM_SPACE,
    TRIMPOS_WIDTH = 8,
//
    LABEL2_X      = X_OFFSET,
    LABEL2_WIDTH  = 0,
    TEXTSEL2_X    = X_OFFSET + 13 * ITEM_SPACE,
    TEXTSEL2_WIDTH= 9 * ITEM_SPACE,
//
    BUTTON2_X     = X_OFFSET + 13 * ITEM_SPACE,
    BUTTON2_Y     = 0,
    BUTTON2_WIDTH = 8 * ITEM_SPACE,
};
#include "../128x64x1/trim_page.c"
