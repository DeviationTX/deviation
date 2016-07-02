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

#if HAS_STANDARD_GUI
#include "../../common/standard/_throhold_page.c"

void PAGE_ThroHoldInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader(_tr("Throttle hold"));

    u8 y = HEADER_HEIGHT + 1;
    GUI_CreateLabelBox(&gui->enlbl, 0, y, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Thr hold"));
    u8 w = 40;
    GUI_CreateTextSelectPlate(&gui->en, 75, y, w, LINE_HEIGHT, &DEFAULT_FONT, NULL, throhold_cb,  NULL);

    y += LINE_SPACE;
    GUI_CreateLabelBox(&gui->valuelbl, 0, y, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Hold position"));
    GUI_CreateTextSelectPlate(&gui->value, 75, y, w, LINE_HEIGHT, &DEFAULT_FONT, NULL, holdpostion_cb,  NULL);

    GUI_Select1stSelectableObj();

}

#endif //HAS_STANDARD_GUI
