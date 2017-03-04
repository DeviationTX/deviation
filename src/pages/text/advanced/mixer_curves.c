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
#include "../pages.h"

enum {
    NAME_X = ITEM_SPACE,
    NAME_W = 10*ITEM_SPACE,
    SAVE_X = LCD_WIDTH - 10*ITEM_SPACE,
    SAVE_W = 8*ITEM_SPACE,
    UNDERLINE = 0,
    LABEL_X = ITEM_SPACE,
    LABEL_W = 10*ITEM_SPACE,
    LABEL1_W = 8*ITEM_SPACE,
    LABEL2_W = 8*ITEM_SPACE,
    TEXTSEL1_X = 12*ITEM_SPACE,
    TEXTSEL1_W = 6*ITEM_SPACE,
    TEXTSEL2_X = 12*ITEM_SPACE,
    TEXTSEL2_W = 6*ITEM_SPACE,
    VALUE_X    = 8*ITEM_SPACE,
    VALUE_Y_OFFSET = 0,
    GRAPH_X = 44,
    GRAPH_Y = 6,
    GRAPH_W = 6,
    GRAPH_H = 4,
};
#include "../../128x64x1/advanced/mixer_curves.c"
