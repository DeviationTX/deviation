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

struct {
    void(*return_page)(u8 *);
    const char *(*text_cb)(u8 idx);
    guiObject_t *listbox;
    guiObject_t *textsel;
    u8 *list;
    u8 count;
    u8 selected;
    u8 max;
    u8 copyto;
} rl;
enum {
    MOVE_UP,
    MOVE_DOWN,
    APPLY,
    INSERT,
    REMOVE,
};

static const char *string_cb(u8 idx, void *data)
{
    (void)data;
    return rl.text_cb(rl.list[idx]);
}

static void select_cb(guiObject_t *obj, u16 sel, void *data)
{
    (void)obj;
    (void)data;
    if(sel < rl.count)  {
        rl.copyto = sel;
        rl.selected = sel;
        GUI_Redraw(rl.textsel);
    } else {
        GUI_ListBoxSelect(rl.listbox, rl.selected);
    }
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    GUI_RemoveAllObjects();
    if (data) {
        rl.return_page(rl.list);
    }
    rl.return_page(NULL);
}

static const char *show_button_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    switch((long)(data)) {
        case MOVE_UP:   return _tr("Move Up");
        case MOVE_DOWN: return _tr("Move Down");
        case APPLY:     return _tr("Copy To");
        case INSERT:    return _tr("Insert");
        case REMOVE:    return _tr("Remove");
    }
    return "";
}

void press_button_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 tmp;
    switch((long)data) {
    case MOVE_UP:
        if(rl.selected > 0) {
            tmp = rl.list[rl.selected-1];
            rl.list[rl.selected-1] = rl.list[rl.selected];
            rl.list[rl.selected] = tmp;
            GUI_ListBoxSelect(rl.listbox, rl.selected - 1);
        }
        break;
    case MOVE_DOWN:
        if(rl.selected < rl.count - 1) {
            tmp = rl.list[rl.selected+1];
            rl.list[rl.selected+1] = rl.list[rl.selected];
            rl.list[rl.selected] = tmp;
            GUI_ListBoxSelect(rl.listbox, rl.selected + 1);
        }
        break;
    case APPLY:
        if(rl.selected != rl.copyto)
            rl.list[rl.copyto] = rl.list[rl.selected];
        GUI_Redraw(rl.listbox);
        break;
    case INSERT:
        if(rl.count < rl.max) {
            for(tmp = rl.count; tmp > rl.selected; tmp--)
                rl.list[tmp] = rl.list[tmp-1];
            rl.count++;
            rl.list[rl.selected] = 255;
            GUI_ListBoxSelect(rl.listbox, rl.selected + 1);
        }
        break;
    case REMOVE:
        if(rl.count > 1) {
            for(tmp = rl.selected; tmp < rl.count; tmp++)
                rl.list[tmp] = rl.list[tmp+1];
            rl.list[rl.count] = 0;
            rl.count--;
            if(rl.selected == rl.count)
                rl.selected--;
            GUI_ListBoxSelect(rl.listbox, rl.selected);
        }
        break;
    }
    return;
}

const char *copy_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    rl.copyto = GUI_TextSelectHelper(rl.copyto, 0, rl.count - 1, dir, 1, 5, NULL);
    return(rl.text_cb(rl.list[rl.copyto]));
}
void PAGE_ShowReorderList(u8 *list, u8 count, u8 selected, u8 max_allowed, const char *(*text_cb)(u8 idx), void(*return_page)(u8 *))
{
    rl.return_page = return_page;
    rl.list = list;
    rl.selected = selected;
    rl.copyto = selected;
    rl.count = count;
    rl.text_cb = text_cb;
    rl.max = max_allowed;
    if (rl.max < count)
        rl.max = count;

    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    int i;
    for(i = 0; i < rl.max; i++) {
        if (i < count)
            list[i] = i+1;
        else
            list[i] = 0;
    }
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
    GUI_CreateButton(8, 40, BUTTON_96x16, show_button_cb, 0x0000, press_button_cb, (void *)MOVE_UP);
    GUI_CreateButton(8, 60, BUTTON_96x16, show_button_cb, 0x0000, press_button_cb, (void *)MOVE_DOWN);

    rl.textsel = GUI_CreateTextSelect(8, 90, TEXTSELECT_96, 0x0000, NULL, copy_val_cb, NULL);
    GUI_CreateButton(8, 110, BUTTON_96x16, show_button_cb, 0x0000, press_button_cb, (void *)APPLY);
    if (max_allowed) {
        GUI_CreateButton(8, 140, BUTTON_96x16, show_button_cb, 0x0000, press_button_cb, (void *)INSERT);
        GUI_CreateButton(8, 160, BUTTON_96x16, show_button_cb, 0x0000, press_button_cb, (void *)REMOVE);
    }
    rl.listbox = GUI_CreateListBox(112, 40, 200, 192, rl.max, selected, string_cb, select_cb, NULL, NULL);
}
