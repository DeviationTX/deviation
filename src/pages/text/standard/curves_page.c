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
#include "mixer.h"
#include "mixer_standard.h"
#include "standard.h"

#define OVERRIDE_PLACEMENT
enum {
    MESSAGE_Y    = 1*LINE_HEIGHT,
    HEADER1_X    = 7*ITEM_SPACE,
    HEADER1_W    = 10*ITEM_SPACE,
    HEADER2_X    = 19*ITEM_SPACE,
    HEADER2_W    = 4*ITEM_SPACE,
    GRAPH_X      = 46,
    GRAPH_Y      = 5,
    GRAPH_W      = 6,
    GRAPH_H      = 4,
};
#include "../../128x64x1/standard/curves_page.c"
