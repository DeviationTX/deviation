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

#define X_OFFSET (ITEM_SPACE*2 - 2)
enum {
    LABEL_X      = X_OFFSET + 10 * ITEM_SPACE,
    ROW1_X       = X_OFFSET,
    ROW1_W       = 4 * ITEM_SPACE,
    ROW2_X       = X_OFFSET + 5 * ITEM_SPACE,
    BUTTON_W     = 9 * ITEM_SPACE,
    SAVE_X       = X_OFFSET + 10 * ITEM_SPACE,
    SAVE_W       = 5 * ITEM_SPACE,
    LABEL_WIDTH  = (LCD_WIDTH-X_OFFSET + 10 * ITEM_SPACE)
};
#include "../128x64x1/reorder_list.c"
