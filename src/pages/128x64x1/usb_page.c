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
    LABEL_X      = 0,
    LABEL_WIDTH  = LCD_WIDTH,
    LABEL_Y      = 15,
    LABEL_HEIGHT = 40,
};
#endif //OVERRIDE_PLACEMENT

#include "../common/_usb_page.c"

static struct usb_obj * const gui = &gui_objs.u.usb;

static void _draw_page(u8 enable)
{
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("USB"));

    snprintf(tempstring, sizeof(tempstring), "%s %s",
            _tr("Press ENT to turn \nUSB drive"),
            enable == 0 ? _tr("On") : _tr("Off"));
    GUI_CreateLabelBox(&gui->label, LABEL_X, LABEL_Y, LABEL_WIDTH, LABEL_HEIGHT, &DEFAULT_FONT, NULL, NULL, tempstring);
}


void PAGE_USBEvent()
{
}
