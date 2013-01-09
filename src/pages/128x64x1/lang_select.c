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
#include "config/tx.h"
#include <string.h>

#include "../common/_lang_select.c"

#define VIEW_ID 0
static u8 _action_cb(u32 button, u8 flags, void *data);
static const char *_string_cb(guiObject_t *obj, const void *data);
static s8 current_selected;
static s16 view_origin_relativeY = 0;

#define gui (&gui_objs.u.lang)
void LANGPage_Select(void(*return_page)(int page))
{
    PAGE_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(1);
    cp->return_page = return_page;
    current_selected = 0; // don't use cp->selected

    PAGE_ShowHeader(_tr("Press ENT to change:"));

    cp->total_items = 1;
    if (FS_OpenDir("language")) {
        char filename[13];
        int type;
        while((type = FS_ReadDir(filename)) != 0) {
            if (type == 1 && strncasecmp(filename, "lang", 4) == 0) {
                cp->total_items++;
            }
        }
        FS_CloseDir();
    }

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 space = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - view_origin_absoluteY ,
            view_origin_absoluteX, view_origin_absoluteY);
    u8 y = 0;
    guiObject_t *obj;
    for (u8 i = 0; i < cp->total_items; i++) {
        obj = GUI_CreateLabelBox(&gui->label[i], GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y),
            0, ITEM_HEIGHT, &DEFAULT_FONT, _string_cb, NULL, (void *)(long)i);
        GUI_SetSelectable(obj, 1);
        if (i == 0)
            GUI_SetSelected(obj);
        y += space;
    }
    //GUI_CreateListBox(112, 40, 200, 192, num_lang, cp->selected, string_cb, select_cb, NULL, NULL); */

    // The following items are not draw in the logical view;
    if (cp->total_items > PAGE_ITEM_MAX)
        GUI_CreateScrollbar(&gui->scroll, LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT- ITEM_HEIGHT, cp->total_items, NULL, NULL, NULL);
    if ( Transmitter.language > 0)
        PAGE_NavigateItems(Transmitter.language, VIEW_ID, cp->total_items, &current_selected, &view_origin_relativeY, &gui->scroll);
}

const char *_string_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    return string_cb(idx, NULL);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_TxConfigureInit(-1);
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            CONFIG_ReadLang(current_selected);
            cp->return_page(0);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            //_navigate_items(-1);
            PAGE_NavigateItems(-1, VIEW_ID, cp->total_items, &current_selected, &view_origin_relativeY, &gui->scroll);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            PAGE_NavigateItems(1, VIEW_ID, cp->total_items, &current_selected, &view_origin_relativeY, &gui->scroll);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
