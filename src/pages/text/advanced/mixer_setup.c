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
#include "config/model.h"

enum {
   TYPE_X     = 10*ITEM_SPACE,
   TYPE_W     = 7*ITEM_SPACE,
   SAVE_X     = LCD_WIDTH - 6*ITEM_SPACE,
   SAVE_W     = 6*ITEM_SPACE,
   LABEL_X    = 1*ITEM_SPACE,
   LABEL_W    = 9*ITEM_SPACE,
   TEXTSEL_X  = 12*ITEM_SPACE,
   TEXTSEL_W  = 8*ITEM_SPACE,
   GRAPH_X    = 46,
   GRAPH_Y    = 6,
   GRAPH_W    = 6,
   GRAPH_H    = 4,
   LEFT_VIEW_WIDTH   = 23*ITEM_SPACE,
   RIGHT_VIEW_HEIGHT = 12,
   LINES_PER_ROW     = 1,
   UNDERLINE         = 0,
};

#include "../../128x64x1/advanced/mixer_setup.c"
