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
#include "config/model.h"
#include "config/ini.h"
#include <stdlib.h>

#include "../common/_model_loadsave.c"

static u8 _action_cb(u32 button, u8 flags, void *data);

static void _show_buttons(int loadsave)
{
    PAGE_SetActionCB(_action_cb);
    u8 w = 40;
    GUI_CreateButtonPlateText(LCD_WIDTH -w -5, 0, w, ITEM_HEIGHT, &DEFAULT_FONT, show_loadsave_cb, 0x0000, okcancel_cb, (void *)(loadsave+1L));
}

static void _show_list(int loadsave,u8 num_models)
{
    GUI_CreateListBoxPlateText(0, ITEM_HEIGHT + 2, LCD_WIDTH, LCD_HEIGHT - ITEM_HEIGHT - 2, num_models, mp->selected-1, &DEFAULT_FONT,
            string_cb, select_cb, NULL, (void *)(long)loadsave);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ModelInit(-1);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
