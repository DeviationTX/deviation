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
//#include "icons.h"

enum {
    COL1_X     = 0*ITEM_SPACE,
    COL1_W     = 7*ITEM_SPACE,
    COL2_X     = 8*ITEM_SPACE,
    COL2_W     = 8*ITEM_SPACE,
    COL3_X     = 17*ITEM_SPACE,
    COL3_W     = 8*ITEM_SPACE,
};

#include "../../128x64x1/advanced/mixer_page.c"
