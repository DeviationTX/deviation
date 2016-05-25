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
#include "pages.h"
#include "gui/gui.h"

enum {
    ROW_1_X = 0,
    ROW_1_Y = 15,
    ROW_2_X = 0,
    ROW_2_Y = 30,
    ROW_3_X = 0,
    ROW_3_Y = 45
};
#endif //OVERRIDE_PLACEMENT


static struct usb_page  * const up = &pagemem.u.usb_page;
static struct about_obj * const gui = &gui_objs.u.about;

void PAGE_AboutInit(int page)
{
    (void)page;
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_ABOUT));

    tempstring_cpy((const char *) _tr("Deviation FW version:"));
    GUI_CreateLabelBox(&gui->label[0], ROW_1_X, ROW_1_Y, LCD_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, "www.deviationtx.com");
    GUI_CreateLabelBox(&gui->label[1], ROW_2_X, ROW_2_Y, LCD_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, tempstring);
    GUI_CreateLabelBox(&gui->label[2], ROW_3_X, ROW_3_Y, LCD_WIDTH, LINE_HEIGHT, &MICRO_FONT, NULL, NULL, _tr_noop(DeviationVersion));
}
