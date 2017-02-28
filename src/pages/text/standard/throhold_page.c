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
    HEADER_OFFSET  = LINE_HEIGHT,
    FIELD_X        = 16*ITEM_SPACE,
    FIELD_WIDTH    = 6*ITEM_SPACE,
    LABEL_X        = 0,
    LABEL_WIDTH    = FIELD_X - LABEL_X,
};
#include "../../128x64x1/standard/throhold_page.c"
