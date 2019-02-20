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
#include "mixer.h"
#include "standard.h"
#include "mixer_standard.h"

enum {
    FIELD_X         = 60,
    FIELD_WIDTH     = 68,
    LABEL_X         = 0,
    LABEL_WIDTH     = FIELD_X - LABEL_X - 1,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_STANDARD_GUI
#include "../../common/standard/_swash_page.c"


void PAGE_SwashInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    get_swash();
    PAGE_ShowHeaderWithSize(_tr_noop("SwashType"), LABEL_WIDTH, HEADER_HEIGHT);
    GUI_CreateTextSelectPlate(&gui->type, FIELD_X, 0, FIELD_WIDTH, HEADER_WIDGET_HEIGHT, &TEXTSEL_FONT, NULL, swash_val_cb, NULL); // FIXME: need a special value for header button/textsels

    u8 y = HEADER_HEIGHT + 1;
    const void * swashlbl[3] = { _tr_noop("ELE Mix"), _tr_noop("AIL Mix"), _tr_noop("PIT Mix") };
    for (int i = 0; i < 3; i++) {
        GUI_CreateLabelBox(&gui->lbl[i], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, GUI_Localize, NULL, swashlbl[i]);
        GUI_CreateTextSelectPlate(&gui->mix[i], FIELD_X, y, FIELD_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, swashmix_val_cb, (void *)(uintptr_t)i);
        y += LINE_SPACE;
    }

    update_swashmixes();
    GUI_Select1stSelectableObj();
}

#endif //HAS_STANDARD_GUI
