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
#include "telemetry.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

enum {
    LABEL_X    = 0,
    LABEL_W    = 2*ITEM_SPACE,
    TEXTSEL1_X = 3*ITEM_SPACE,
    TEXTSEL1_W = 7*ITEM_SPACE,
    TEXTSEL2_X = 12*ITEM_SPACE,
    TEXTSEL2_W = 3*ITEM_SPACE,
    TEXTSEL3_X = 17*ITEM_SPACE,
    TEXTSEL3_W = 6*ITEM_SPACE,
    MSG_X      = 10*ITEM_SPACE,
    MSG_Y      = 5*ITEM_SPACE,
};
#include "../128x64x1/telemconfig_page.c"
