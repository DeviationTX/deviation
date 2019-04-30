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
#include "config/model.h"

#define OVERRIDE_PLACEMENT

enum {
    MODE_X        = 2,
    MODE_Y        = HEADER_HEIGHT,
    MODE_WIDTH    = 10 * ITEM_SPACE,
    CHANNEL_X     = 2,
    CHANNEL_Y     = ITEM_HEIGHT * 2,
    CHANNEL_WIDTH = 8 * ITEM_SPACE,
    LENGTH_X      = 2,
    LENGTH_Y      = CHANNEL_Y + ITEM_HEIGHT,
    LENGTH_WIDTH  = CHANNEL_WIDTH,
    PACKET_X      = (LCD_WIDTH == 24) ? 0 : 6,
    PACKET_Y      = LENGTH_Y + ITEM_HEIGHT * 2,
    PACKET_WIDTH  = ITEM_SPACE * 16 + 7,
    PACKET_HEIGHT = LINE_HEIGHT,
    STATUS_Y      = LCD_HEIGHT-2,
};

#include "../128x64x1/xn297dump_page.c"
