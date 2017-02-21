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
#include "pages.h"
#include "config/model.h"
#include "config/ini.h"

#define OVERRIDE_PLACEMENT

enum {
    DIALOG1_X      = 1 * ITEM_SPACE,
    DIALOG1_Y      = 1 * LINE_HEIGHT,
    DIALOG1_WIDTH  = LCD_WIDTH - 2 * ITEM_SPACE,
    DIALOG1_HEIGHT = LCD_HEIGHT - 2 * LINE_HEIGHT,
    //
    DIALOG2_X      = 1 * ITEM_SPACE,
    DIALOG2_Y      = 1 * LINE_HEIGHT,
    DIALOG2_WIDTH  = LCD_WIDTH - 2 * ITEM_SPACE,
    DIALOG2_HEIGHT = LCD_HEIGHT - 2 * LINE_HEIGHT,
    //
    DIALOG3_X      = 1 * ITEM_SPACE,
    DIALOG3_Y      = 1 * LINE_HEIGHT,
    DIALOG3_WIDTH  = LCD_WIDTH - 2 * ITEM_SPACE,
    DIALOG3_HEIGHT = LCD_HEIGHT - 2 * LINE_HEIGHT,
};
#include "../128x64x1/dialogs.c"
