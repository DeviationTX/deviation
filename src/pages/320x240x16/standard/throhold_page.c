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

static void toggle_thold_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    int dir;
    if (Model.limits[mapped_std_channels.throttle].safetysw)
        dir = -1;
    else
        dir = 1;
    throhold_cb(obj, dir, data);
}

void PAGE_ThroHoldInit(int page)
{
    (void)page;
    enum {
        COL1 = (30 + ((LCD_WIDTH - 320) / 2)),
        COL2 = (150 + ((LCD_WIDTH - 320) / 2)),
        ROW1 = (80 + ((LCD_HEIGHT - 240) / 2)),
        ROW2 = (120 + ((LCD_HEIGHT - 240) / 2)),
    };
    PAGE_ShowHeader(NULL);
    PAGE_ShowHeader_SetLabel(STDMIX_TitleString, SET_TITLE_DATA(PAGEID_THROHOLD, SWITCHFUNC_HOLD));
    GUI_CreateLabelBox(&gui->enlbl, COL1, ROW1, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Thr hold"));
    GUI_CreateTextSelect(&gui->en, COL2, ROW1, TEXTSELECT_128, toggle_thold_cb, throhold_cb,  NULL);

    GUI_CreateLabelBox(&gui->valuelbl, COL1, ROW2, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Hold position"));
    GUI_CreateTextSelect(&gui->value, COL2, ROW2, TEXTSELECT_128, NULL, holdpostion_cb,  NULL);
}
#endif //HAS_STANDARD_GUI
