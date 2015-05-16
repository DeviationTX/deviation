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
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"
#include "mixer_standard.h"
#include "standard/standard.h"

#if LCD_WIDTH == 66
//devof12e
enum {
    DEFAULT_TEXTSEL_X = 14*2,
    LEFT_TEXTSEL_X    = 4,
    TEXTSEL_WIDTH     = 22,
    LABEL_X           = 4,
    LABEL_WIDTH       = 14*2,
    BUTTON_X          = 14*2,
    BUTTON_WIDTH      = 14*2,
};
#else
#define TEXTSEL_WIDTH   ((ts_x != LEFT_TEXTSEL_X) ? LCD_WIDTH - ts_x - 1: 12)
//devof7
enum {
    DEFAULT_TEXTSEL_X = 14,
    LEFT_TEXTSEL_X    = 1,
    LABEL_X           = 0,
    LABEL_WIDTH       = 14,
    BUTTON_X          = 14,
    BUTTON_WIDTH      = LCD_WIDTH - 14,
};
#endif
#include "../128x64x1/model_page.c"
