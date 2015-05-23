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
#include "telemetry.h"
#include "gui/gui.h"

enum {
    ITEM1_X      = 2*ITEM_SPACE,
    ITEM1_WIDTH  = 6*ITEM_SPACE,
    ITEM2_X      = 9*ITEM_SPACE,
    ITEM3_X      = 16*ITEM_SPACE,
    ARROW_X      = LCD_WIDTH - 2*ITEM_SPACE,
    ARROW_W      = 2*ITEM_SPACE,
    //
    LBL1_X       = 0,
    LBL1_WIDTH   = 8*ITEM_SPACE,
    LBL2_X       = 9*ITEM_SPACE,
    LBL3_X       = 17*ITEM_SPACE,
    LBL4_X       = 0,
    LBL4_WIDTH   = 4*ITEM_SPACE,
    LBL5_X       = 9*ITEM_SPACE,
    //
    HEADER_X     = 18*ITEM_SPACE,
    HEADER_WIDTH = 9*ITEM_SPACE,
    //
    GPS_X        = 0,
    GPS_WIDTH    = LCD_WIDTH - ARROW_WIDTH - 1*ITEM_SPACE,
    //
    DSM1_X       = 2*ITEM_SPACE,
    DSM1_WIDTH   = 6*ITEM_SPACE,
    DSM2_X       = 12*ITEM_SPACE,
    DSM3_X       = 18*ITEM_SPACE,
    DSM4_X       = 5*ITEM_SPACE,
    DSM5_X       = 18*ITEM_SPACE,
    //
    FRSKY1_X     = 2*ITEM_SPACE,
    FRSKY1_WIDTH = 6*ITEM_SPACE,
    FRSKY2_X     = 9*ITEM_SPACE,
    FRSKY3_X     = 18*ITEM_SPACE,
};

#include "../128x64x1/telemtest_page.c"
