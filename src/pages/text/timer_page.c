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

#define OVERRIDE_PLACEMENT

#define X_OFFSET (ITEM_SPACE * 2 - 2)
enum {
    LABEL_X        = X_OFFSET,
    LABEL_WIDTH    = 10 * ITEM_SPACE,
    TEXTSEL_X      = X_OFFSET + 11 * ITEM_SPACE,
    TEXTSEL_WIDTH  = 10 * ITEM_SPACE,
    RESET_X        = X_OFFSET + 12 * ITEM_SPACE,
    RESET_WIDTH    = 8 * ITEM_SPACE,
    START_WIDTH    = X_OFFSET + 10 * ITEM_SPACE,
};
#include "../128x64x1/timer_page.c"
