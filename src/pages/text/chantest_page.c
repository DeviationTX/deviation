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
#include "config/model.h"

#define RAW_FONT    DEFAULT_FONT.font
#define CHAN_FONT   DEFAULT_FONT.font
enum {
    LABEL_COL1_X = ITEM_SPACE,
    LABEL_COL2_X = 15*ITEM_SPACE,
    LABEL_IDX_W  = 7*ITEM_SPACE,
    LABEL_CHAN_H = LINE_HEIGHT,
    RAW_HEIGHT   = LINE_HEIGHT,
    CHAN_HEIGHT  = LINE_HEIGHT,
    CHAN_X_OFFSET = 8*ITEM_SPACE,
    LABEL_CHAN_W = 5*ITEM_SPACE,
    BAR_W        = 0,
    BAR_H        = 0,
    SCROLLABLE_X = 0,
    ARROW_W      = 2*ITEM_SPACE,
};

#include "../128x64x1/chantest_page.c"
