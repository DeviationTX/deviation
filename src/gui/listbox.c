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
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

#include "_listbox.c"
static u8 scroll_cb(struct guiObject *parent, u8 pos, s8 direction, void *data);
static u8 press_cb(u32 button, u8 flags, void *data);

guiObject_t *GUI_CreateListBox(guiListbox_t *listbox, u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(struct guiObject *obj, u16 selected, void *data),
        void (*longpress_cb)(struct guiObject *obj, u16 selected, void *data),
        void *cb_data)
{
    return GUI_CreateListBoxPlateText(listbox, x, y, width, height, item_count, selected, NULL, LISTBOX_KEY_RIGHTLEFT,
            string_cb, select_cb, longpress_cb, cb_data);
}

guiObject_t *GUI_CreateListBoxPlateText(guiListbox_t *listbox, u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const struct LabelDesc *desc, enum ListBoxNavigateKeyType keyType,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(struct guiObject *obj, u16 selected, void *data),
        void (*longpress_cb)(struct guiObject *obj, u16 selected, void *data),
        void *cb_data)
{
    struct guiObject  *obj = (guiObject_t *)listbox;
    struct guiBox     *box;
    u16 text_w, text_h;
    s16 pos = 0;
    u8 sb_entries;

    box = &obj->box;

    box->x = x;
    box->y = y;
    box->width = width - ARROW_WIDTH;
    box->height = height;

    obj->Type = Listbox;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_SELECTABLE(obj, 1);
    connect_object(obj);

    if (desc != NULL) {
        listbox->style = LISTBOX_DEVO10;
        listbox->desc = *desc;
        LCD_SetFont(listbox->desc.font);
        LCD_GetCharDimensions('A', &text_w, &text_h);
        listbox->text_height = text_h;  //no extra spacing text height in devo10
    } else {
        listbox->style = LISTBOX_OTHERS;
        LCD_SetFont(Display.listbox.font ? Display.listbox.font : DEFAULT_FONT.font);
        LCD_GetCharDimensions('A', &text_w, &text_h);
        listbox->text_height = text_h + LINE_SPACING;  //LINE_SPACING is defined in _gui.h
    }
    listbox->entries_per_page = (height + 2) / listbox->text_height;
    if (listbox->entries_per_page >= item_count) {
        listbox->entries_per_page = item_count;
        sb_entries = item_count;
        pos = 0;
    } else {
        sb_entries = item_count - listbox->entries_per_page;
        if(selected >= 0) {
            if (selected < sb_entries) {
                pos = selected - (listbox->entries_per_page / 2);
                if (pos < 0)
                    pos = 0;
            } else {
                pos = sb_entries;
            }
        }
    }
    listbox->cur_pos = pos;
    listbox->item_count = item_count;
    listbox->selected = selected;
    listbox->key_style = keyType;
    listbox->string_cb = string_cb;
    listbox->select_cb = select_cb;
    listbox->longpress_cb = longpress_cb;
    listbox->cb_data = cb_data;

    // bug fix: the total items of scroll-bar is 0-based while devo10 is 1-based
    // 0-based for sum is not a good idea.
    sb_entries++;
    GUI_CreateScrollbar(&listbox->scrollbar,
              x + width - ARROW_WIDTH,
              y,
              height,
              sb_entries,
              obj,
              scroll_cb, NULL);
    GUI_SetScrollbar(&listbox->scrollbar, pos);
    if (listbox->item_count <= listbox->entries_per_page)
        GUI_SetHidden((guiObject_t *)&listbox->scrollbar, 1);
    if (listbox->key_style == LISTBOX_KEY_RIGHTLEFT)
        BUTTON_RegisterCallback(&listbox->action,
                 CHAN_ButtonMask(BUT_LEFT)
                 | CHAN_ButtonMask(BUT_RIGHT)
                 | CHAN_ButtonMask(BUT_ENTER),
                 BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_PRIORITY,
                 press_cb, obj);
    else
        BUTTON_RegisterCallback(&listbox->action,
                 CHAN_ButtonMask(BUT_UP)
                 | CHAN_ButtonMask(BUT_DOWN)
                 | CHAN_ButtonMask(BUT_ENTER),
                 BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_PRIORITY,
                 press_cb, obj);
    return obj;
}

void GUI_ListBoxSelect(struct guiListbox *listbox, u16 selected)
{
    guiObject_t *obj = (guiObject_t *)listbox;
    listbox->selected = selected;
    if (listbox->select_cb)
        listbox->select_cb(obj, (u16)selected, listbox->cb_data);
    OBJ_SET_DIRTY(obj, 1);
}
    
static u8 scroll_cb(struct guiObject *parent, u8 pos, s8 direction, void *data)
{
    (void)pos;
    (void)data;
    struct guiListbox *listbox = (struct guiListbox *)parent;
    if (direction > 0) {
        s16 new_pos = (s16)listbox->cur_pos + (direction > 1 ? listbox->entries_per_page : 1);
        if (new_pos > listbox->item_count - listbox->entries_per_page)
            new_pos = listbox->item_count - listbox->entries_per_page;
        if(listbox->cur_pos != new_pos) {
            listbox->cur_pos = (u16)new_pos;
            if(listbox->selected < listbox->cur_pos)
                listbox->selected = listbox->cur_pos;
            OBJ_SET_DIRTY(parent, 1);
        }
    } else if (direction < 0) {
        s16 new_pos = (s16)listbox->cur_pos - (direction < -1 ? listbox->entries_per_page : 1);
        if (new_pos < 0)
            new_pos = 0;
        if(listbox->cur_pos != new_pos) {
            listbox->cur_pos = (u16)new_pos;
            if(listbox->selected >= listbox->cur_pos + listbox->entries_per_page)
                listbox->selected = listbox->cur_pos + listbox->entries_per_page - 1;
            OBJ_SET_DIRTY(parent, 1);
        }
    }
    return listbox->cur_pos;
}

void GUI_DrawListbox(struct guiObject *obj, u8 redraw_all)
{
    u8 i;
    struct guiListbox *listbox = (struct guiListbox *)obj;
    unsigned int font;
    if (listbox->item_count <= listbox->entries_per_page)
        GUI_SetHidden((guiObject_t *)&listbox->scrollbar, 1);
    else
        GUI_SetHidden((guiObject_t *)&listbox->scrollbar, 0);
    if (listbox->style == LISTBOX_DEVO10) {
        font = listbox->desc.font;
   } else {
        font = Display.listbox.font ? Display.listbox.font : DEFAULT_FONT.font;
    }
    if (redraw_all) {
        LCD_FillRect(obj->box.x, obj->box.y, obj->box.width, obj->box.height, FILL);
    }
    LCD_SetXY(obj->box.x + 5, obj->box.y + LINE_SPACING -1);
    if(listbox->selected >= listbox->cur_pos && listbox->selected < listbox->cur_pos + listbox->entries_per_page) {
        // Bug fix: each line of row contains both text and line-spacing, so the height should take line-spacing into account
        if (listbox->style == LISTBOX_DEVO10) {
            if (obj != objSELECTED)  // only draw a box when the listbox is not selected
                LCD_DrawRect(obj->box.x,
                    obj->box.y + (listbox->selected - listbox->cur_pos) * (listbox->text_height + LINE_SPACING),
                    obj->box.width, listbox->text_height, SELECT);
            else
                LCD_FillRect(obj->box.x,
                    obj->box.y + (listbox->selected - listbox->cur_pos) * (listbox->text_height + LINE_SPACING),
                    obj->box.width, listbox->text_height, SELECT);
        } else {
            LCD_FillRect(obj->box.x,
            obj->box.y + (listbox->selected - listbox->cur_pos) * (listbox->text_height),
            obj->box.width, listbox->text_height, SELECT);
        }
    }
    LCD_SetFont(font);
    for(i = 0; i < listbox->entries_per_page; i++) {
        const char *str = listbox->string_cb(i + listbox->cur_pos, listbox->cb_data);
        if (listbox->style == LISTBOX_DEVO10) {
            LCD_SetFontColor((i + listbox->cur_pos == listbox->selected && obj == objSELECTED)? SELECT_TXT : TEXT);
        } else
            LCD_SetFontColor(i + listbox->cur_pos == listbox->selected ? SELECT_TXT : TEXT);
        LCD_PrintString(str);
        LCD_PrintString("\n");
    }
}

u8 GUI_TouchListbox(struct guiObject *obj, struct touch *coords, u8 long_press)
{
    struct guiListbox *listbox = (struct guiListbox *)obj;
    struct guiBox box;
    u8 i;
    box.x = obj->box.x;
    box.y = obj->box.y;
    box.width = obj->box.width;
    box.height = listbox->text_height;
    for (i = 0; i < listbox->entries_per_page; i++) {
        box.y = obj->box.y + i * listbox->text_height;
        if (coords_in_box(&box, coords)) {
            u8 selected = i + listbox->cur_pos;
            if (selected != listbox->selected) {
                listbox->selected = selected;
                if (listbox->select_cb)
                    listbox->select_cb(obj, (u16)selected, listbox->cb_data);
                OBJ_SET_DIRTY(obj, 1);
                return 1;
            } else if (long_press && listbox->longpress_cb) {
                listbox->longpress_cb(obj, (u16)selected, listbox->cb_data);
            }
            return 0;
        }
    }
    return 0;
}

static u8 press_cb(u32 button, u8 flags, void *data)
{   // fix bug for issue #81: DEVO10: Model list should be browsable with UP/DOWN, so the listbox can change navigate key-sets now
    struct guiObject *obj = (struct guiObject *)data;
    struct guiListbox *listbox = (struct guiListbox *)obj;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        // devo10's right/left buttons are upside down compared with devo8's
        u8 move_down_button = BUT_RIGHT;
        u8 move_up_button = BUT_LEFT;
        if (listbox->style == LISTBOX_DEVO10) {
            if (listbox->key_style != LISTBOX_KEY_RIGHTLEFT) {
                move_down_button = BUT_DOWN;
                move_up_button = BUT_UP;
            }
        }
        if (CHAN_ButtonIsPressed(button, move_down_button)) {
            if (listbox->selected < listbox->item_count - 1) {
                listbox->selected++;
                if (listbox->selected >= listbox->cur_pos + listbox->entries_per_page)
                    GUI_SetScrollbar(&listbox->scrollbar, scroll_cb(obj, 0, 1, NULL));
                if (listbox->select_cb)
                    listbox->select_cb(obj, (u16)listbox->selected, listbox->cb_data);
                OBJ_SET_DIRTY(obj, 1);
            }
        } else if (CHAN_ButtonIsPressed(button, move_up_button)) {
            if (listbox->selected > 0) {
                listbox->selected--;
                if (listbox->selected < listbox->cur_pos)
                    GUI_SetScrollbar(&listbox->scrollbar, scroll_cb(obj, 0, -1, NULL));
                if (listbox->select_cb)
                    listbox->select_cb(obj, (u16)listbox->selected, listbox->cb_data);
                OBJ_SET_DIRTY(obj, 1);
            }
        } else if(CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            if (objSELECTED != obj)
                return 0; // when the listbox is not selected ,don't catch ENTER key
            if (listbox->longpress_cb)
                listbox->longpress_cb(obj, (u16)listbox->selected, listbox->cb_data);
            else
                listbox->select_cb(obj, (u16)listbox->selected, listbox->cb_data);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;

}

