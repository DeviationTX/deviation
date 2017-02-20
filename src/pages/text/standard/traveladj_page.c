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
#include "../pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "standard.h"

#define OVERRIDE_PLACEMENT
enum {
    LABEL_X        = 9*ITEM_SPACE,
    LABEL_WIDTH    = 5*ITEM_SPACE,
    LABEL_OFFSET   = 3*ITEM_SPACE,
    HEADER_X       = 9*ITEM_SPACE,
    HEADER_WIDTH   = 6*ITEM_SPACE,
    HEADER_OFFSET1 = ITEM_SPACE,
    HEADER_OFFSET2 = 3*ITEM_SPACE,
};
#include "../../128x64x1/standard/traveladj_page.c"
