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
#include "gui/gui.h"
#include "config/model.h"
#include "../pages.h"

#define OVERRIDE_PLACEMENT
enum {
    //"Reverse", "Subtrim" and "Fail-safe" pages
    FIELD_X        = 10*ITEM_SPACE,
    FIELD_WIDTH    = 10*ITEM_SPACE,
    LABEL_X        = 0,
    LABEL_WIDTH    = FIELD_X - LABEL_X,
    //"Throttle curve" and "Pitch curve" pages XY-graph points
    LINE_Y         = 2*LINE_HEIGHT,
    WIDTH1         = 3*ITEM_SPACE,
    WIDTH2         = 5*ITEM_SPACE,
    WIDTH2_ADD     = 3*ITEM_SPACE,
    LINE_H         = LINE_HEIGHT,
    LINE_H_OFFS    = 0,
    M_LABEL_X      = 5*ITEM_SPACE,
    M_LEBEL_Y_OFFS = LINE_HEIGHT,
};
#include "../../128x64x1/standard/common_standard.c"
