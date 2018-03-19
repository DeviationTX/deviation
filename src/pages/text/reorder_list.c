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

#define X_OFFSET ITEM_SPACE
enum {
    MOVE_UP_X      = X_OFFSET,
    MOVE_UP_W      = 6*ITEM_SPACE,
    MOVE_DOWN_X    = X_OFFSET + 7*ITEM_SPACE,
    MOVE_DOWN_W    = 6*ITEM_SPACE,
    VALUE_X        = X_OFFSET,
    VALUE_W        = 8*ITEM_SPACE,
    APPLY_X        = X_OFFSET,
    APPLY_W        = 8*ITEM_SPACE,
    INSERT_X       = X_OFFSET,
    INSERT_W       = 6*ITEM_SPACE,
    REMOVE_X       = X_OFFSET + 7*ITEM_SPACE,
    REMOVE_W       = 6*ITEM_SPACE,
    SAVE_X         = X_OFFSET,
    SAVE_W         = 8*ITEM_SPACE,
    SAVE_Y_OFFS    = 0,
    LIST_X         = X_OFFSET + 14*ITEM_SPACE,
    LIST_W         = LCD_WIDTH - LIST_X - ITEM_SPACE,
};
#include "../128x64x1/reorder_list.c"
