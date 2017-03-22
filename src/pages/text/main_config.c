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
#include "telemetry.h"

enum {
    NEWTEXTSEL_X = ITEM_SPACE,
    NEWTEXTSEL_W = 9*ITEM_SPACE,
    NEWBUTTON_X = 13*ITEM_SPACE,
    NEWBUTTON_W =  9*ITEM_SPACE,
    QPBUTTON_X  = 13*ITEM_SPACE,
    QPBUTTON_W  =  9*ITEM_SPACE,
    MENULABEL_X =  2*ITEM_SPACE,
    MENULABEL_W = 19*ITEM_SPACE,
    MENUTEXT_X  =  1*ITEM_SPACE,
    MENUTEXT_W  = 21*ITEM_SPACE,
    #define LINE_OFFSET 1
    ELEMBUT_X   =  1*ITEM_SPACE,
    ELEMBUT_W   =  9*ITEM_SPACE,
    ELEMLBL_X   =  1*ITEM_SPACE,
    ELEMLBL_W   =  9*ITEM_SPACE,
    ELEMTXT_X   = 13*ITEM_SPACE,
    ELEMTXT_W   =  9*ITEM_SPACE,
};
#include "../128x64x1/main_config.c"
