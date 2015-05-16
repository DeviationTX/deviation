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

#define OVERRIDE_PLACEMENT
enum {
    ROW_1_X = 0,
    ROW_1_Y = 1*LINE_HEIGHT,
    ROW_2_X = 0,
    ROW_2_Y = 4 * LINE_HEIGHT,
    ROW_3_X = 0,
    ROW_3_Y = 5 * LINE_HEIGHT,
};
#include "../128x64x1/about_page.c"
