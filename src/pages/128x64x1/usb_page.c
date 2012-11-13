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
static u8 _action_cb(u32 button, u8 flags, void *data);

static void _draw_page(u8 enable)
{
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("USB"));
    PAGE_SetActionCB(_action_cb);

    sprintf(up->tmpstr, "%s%s",
            _tr("Press ENT to turn \nUSB drive:"),
            enable == 0 ? _tr("On") : _tr("Off"));
    GUI_CreateLabelBox(0, 15, LCD_WIDTH, 40, &DEFAULT_FONT, NULL, NULL, up->tmpstr);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("MainMenu", 0);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
