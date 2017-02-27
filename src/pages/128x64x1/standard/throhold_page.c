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
    HEADER_OFFSET  = 1,
    FIELD_X        = 75,
    FIELD_WIDTH    = 40,
    LABEL_X        = 0,
    LABEL_WIDTH    = FIELD_X - LABEL_X,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_STANDARD_GUI
#include "../../common/standard/_throhold_page.c"

void PAGE_ThroHoldInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader(_tr("Throttle hold"));

    u8 y = HEADER_HEIGHT + HEADER_OFFSET;
    GUI_CreateLabelBox(&gui->enlbl, LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, NULL, NULL, _tr("Thr hold"));
    GUI_CreateTextSelectPlate(&gui->en, FIELD_X, y, FIELD_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, throhold_cb,  NULL);

    y += LINE_SPACE;
    GUI_CreateLabelBox(&gui->valuelbl, LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, NULL, NULL, _tr("Hold position"));
    GUI_CreateTextSelectPlate(&gui->value, FIELD_X, y, FIELD_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, holdpostion_cb,  NULL);

    GUI_Select1stSelectableObj();

}

#endif //HAS_STANDARD_GUI
