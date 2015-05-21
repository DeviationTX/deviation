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
//#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

#define OVERRIDE_PLACEMENT
extern const char *TGLICO_font_cb(guiObject_t *obj, const void *data);
extern void TGLICO_LoadFonts();

#if LCD_WIDTH == 66
//devof12e
enum {
     VTRIM_W      =  2,
     VTRIM_H      = 18,
     HTRIM_W      = 22,
     HTRIM_H      =  2,
     MODEL_ICO_W  =  6,
     MODEL_ICO_H  =  4,
     BOX_W        = 10,
     SMALLBOX_H   =  2,
     BIGBOX_H     =  2,
     GRAPH_W      = (VTRIM_W),
     GRAPH_H      = (VTRIM_H / 2),
     BATTERY_W    = 10,
     BATTERY_H    = 2,
     TXPOWER_W    = 10,
     TXPOWER_H    = 2,
//
    MODEL_NAME_X  = 0,
    MODEL_NAME_Y  = 0,
};
#else
#define TGLICO_LoadFonts() (void)1
enum {
     VTRIM_W      =  1,
     VTRIM_H      =  9,
     HTRIM_W      = 11,
     HTRIM_H      =  1,
     MODEL_ICO_W  = 52,
     MODEL_ICO_H  = 36,
     BOX_W        =  6,
     SMALLBOX_H   = 10,
     BIGBOX_H     = 14,
     GRAPH_W      = (VTRIM_W),
     GRAPH_H      = (VTRIM_H / 2),
     BATTERY_W    = 4,
     BATTERY_H    = 1,
     TXPOWER_W    = 2,
     TXPOWER_H    = 1,
//
    MODEL_NAME_X  = 0,
    MODEL_NAME_Y  = 0,
};
#endif
#include "../128x64x1/main_page.c"
