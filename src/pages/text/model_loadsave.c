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
#include "config/model.h"
#include "config/ini.h"
#include <stdlib.h>

#define OVERRIDE_PLACEMENT
enum {
    ICON_X      = LCD_WIDTH/2 - 6*ITEM_SPACE,
    ICON_W      = 12*ITEM_SPACE,
    IMAGE_X     = LCD_WIDTH/2 - 4*ITEM_SPACE,
    IMAGE_Y     = LCD_HEIGHT - 9*ITEM_SPACE,
    IMAGE_W     = 6,
    IMAGE_H     = 4,
};
#include "../128x64x1/model_loadsave.c"
