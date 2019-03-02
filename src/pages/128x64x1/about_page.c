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

static struct about_obj * const gui = &gui_objs.u.about;

void PAGE_AboutInit(int page)
{
    (void)page;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_ABOUT));

    BeginGridLayout(4, 1) {
        GUI_CreateLabelBox(&gui->label[0], Grid_XY(1, 1), LCD_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, "www.deviationtx.com");
        GUI_CreateLabelBox(&gui->label[1], Grid_XY(2, 1), LCD_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, GUI_Localize, NULL, _tr_noop("Deviation FW version:"));
        GUI_CreateLabelBox(&gui->label[2], Grid_XY(3, 1), LCD_WIDTH, LINE_HEIGHT, &TINY_FONT, GUI_Localize, NULL, _tr_noop(DeviationVersion));
    }
}
