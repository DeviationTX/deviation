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

#include "../common/_usb_page.c"

#define gui (&gui_objs.u.usb)

static void _draw_page(u8 enable)
{
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_USB));

    GUI_CreateLabelBox(&gui->headline, LCD_WIDTH/2-100, 60, 200, 40, &MODELNAME_FONT, NULL, NULL, "www.deviationtx.com");
    sprintf(up->tmpstr, "%s\n%s\n\n%s... %s\n%s %s",
            _tr("Deviation FW version:"), DeviationVersion,
            _tr("USB Filesystem is currently "), enable == 0 ? _tr("Off") : _tr("On"),
            _tr("Press 'Ent' to turn USB Filesystem"),
            enable == 0 ? _tr("On") : _tr("Off"));
    GUI_CreateLabelBox(&gui->msg, LCD_WIDTH/2-126, 120, 252, LCD_HEIGHT-120, &BOLD_FONT, NULL, NULL, up->tmpstr);
}
