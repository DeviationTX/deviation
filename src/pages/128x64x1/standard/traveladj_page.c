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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "../pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "standard.h"

enum {
    HEADER1_X      = 52,
    HEADER1_WIDTH  = 35,
    HEADER2_X      = HEADER1_X + HEADER1_WIDTH + 5,
    HEADER2_WIDTH  = HEADER1_WIDTH,
    FIELD1_X       = 50,
    FIELD1_WIDTH   = 35,
    FIELD2_X       = FIELD1_X + FIELD1_WIDTH + 3,
    FIELD2_WIDTH   = FIELD1_WIDTH,
    LABEL_X        = 0,
    LABEL_WIDTH    = FIELD1_X - LABEL_X,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_STANDARD_GUI
#include "../../common/standard/_traveladj_page.c"

static struct stdtravel_obj * const gui = &gui_objs.u.stdtravel;

static u16 current_selected = 0;

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    mp->limit = MIXER_GetLimit(absrow);
    GUI_CreateLabelBox(&gui->chan[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, STDMIX_channelname_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->dn[relrow], FIELD1_X, y,
            FIELD1_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, traveldown_cb, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->up[relrow], FIELD2_X, y,
            FIELD2_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, travelup_cb, (void *)(long)absrow);
    return 2;
}

void PAGE_TravelAdjInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader(("")); // draw a underline only
    GUI_CreateLabelBox(&gui->dnlbl, HEADER1_X, 0, HEADER1_WIDTH, LINE_HEIGHT, &TITLE_FONT, NULL, NULL, _tr("Down"));
    GUI_CreateLabelBox(&gui->uplbl, HEADER2_X, 0, HEADER2_WIDTH, LINE_HEIGHT, &TITLE_FONT, NULL, NULL, _tr("Up"));

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, Model.num_channels, row_cb, NULL, NULL, NULL);

    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

#endif //HAS_STANDARD_GUI
