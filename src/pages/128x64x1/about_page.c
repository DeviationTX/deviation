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

static u8 _action_cb(u32 button, u8 flags, void *data);

static struct usb_page * const up = &pagemem.u.usb_page;
void PAGE_AboutInit(int page)
{
    (void)page;
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("About Deviation"));
    PAGE_SetActionCB(_action_cb);

    strcpy(up->tmpstr, (const char *) _tr("Deviation FW version:"));
    GUI_CreateLabelBox(0, 15, LCD_WIDTH, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, "www.deviationtx.com");
    GUI_CreateLabelBox(0, 30, LCD_WIDTH, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, up->tmpstr);
    GUI_CreateLabelBox(0, 45, LCD_WIDTH, ITEM_HEIGHT, &TINY_FONT, NULL, NULL, _tr_noop(DeviationVersion));
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("SubMenu", sub_menu_item);
        }
    }
    return 1;
}
