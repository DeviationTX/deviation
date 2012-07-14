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

#include "target.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

guiObject_t *GUI_CreateListBox(u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(struct guiObject *obj, u16 selected, void *data),
        void (*longpress_cb)(struct guiObject *obj, u16 selected, void *data),
        void *cb_data)
{
    struct guiObject  *obj = GUI_GetFreeObj();
    struct guiListbox *listbox;
    struct guiBox     *box;
    u16 text_w, text_h;
    s16 pos;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    listbox = &obj->o.listbox;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Listbox;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    LCD_GetCharDimensions('A', &text_w, &text_h);
    listbox->text_height = text_h + 2;  //LINE_SPACING = 2
    listbox->entries_per_page = (height + 2) / listbox->text_height;
    if (listbox->entries_per_page > item_count)
        listbox->entries_per_page = item_count;
    listbox->item_count = item_count;
    listbox->cur_pos = 0;
    if(selected >= 0) {
        pos = selected - (listbox->entries_per_page / 2);
        if (pos < 0)
            pos = 0;
        listbox->cur_pos = pos;
    }
    listbox->selected = selected;
    
    listbox->string_cb = string_cb;
    listbox->select_cb = select_cb;
    listbox->longpress_cb = longpress_cb;
    listbox->cb_data = cb_data;

    return obj;
}

void GUI_DrawListbox(struct guiObject *obj, u8 redraw_all)
{
    #define BAR_HEIGHT 10
    #define FILL        Display.listbox.bg_color    // RGB888_to_RGB565(0xaa, 0xaa, 0xaa)
    #define TEXT        Display.listbox.fg_color    // 0x0000
    #define SELECT      Display.listbox.bg_select   // RGB888_to_RGB565(0x44, 0x44, 0x44)
    #define SELECT_TXT  Display.listbox.fg_select   // 0xFFFF
    #define BAR_BG      Display.listbox.bg_bar      // RGB888_to_RGB565(0x44, 0x44, 0x44)
    #define BAR_FG      Display.listbox.fg_bar      // RGB888_to_RGB565(0xaa, 0xaa, 0xaa)
    
    u8 i, old;
    u8 bar;
    struct guiListbox *listbox = &obj->o.listbox;
    if (redraw_all) {
        LCD_FillRect(obj->box.x, obj->box.y, obj->box.width - ARROW_WIDTH, obj->box.height, FILL);
        GUI_DrawImageHelper(obj->box.x + obj->box.width - ARROW_WIDTH, obj->box.y,
                            ARROW_UP, DRAW_NORMAL);
        GUI_DrawImageHelper(obj->box.x + obj->box.width - ARROW_WIDTH,
                            obj->box.y + obj->box.height - ARROW_HEIGHT,
                            ARROW_DOWN, DRAW_NORMAL);
    }
    u16 denom = (listbox->item_count == listbox->entries_per_page) ?  1 : listbox->item_count - listbox->entries_per_page;
    bar = listbox->cur_pos * (obj->box.height - 2 * ARROW_HEIGHT - BAR_HEIGHT) / denom;
    LCD_FillRect(obj->box.x + obj->box.width - ARROW_WIDTH,
                 obj->box.y + ARROW_HEIGHT, ARROW_WIDTH,
                 obj->box.height - 2 * ARROW_HEIGHT, BAR_BG);
    LCD_FillRect(obj->box.x + obj->box.width - ARROW_WIDTH,
                 obj->box.y + ARROW_HEIGHT + bar,
                 ARROW_WIDTH, BAR_HEIGHT, BAR_FG);
    LCD_SetXY(obj->box.x + 5, obj->box.y + 1);
    if(listbox->selected >= listbox->cur_pos && listbox->selected < listbox->cur_pos + listbox->entries_per_page)
        LCD_FillRect(obj->box.x,
                     obj->box.y + (listbox->selected - listbox->cur_pos) * listbox->text_height,
                     obj->box.width - ARROW_WIDTH, listbox->text_height, SELECT);
    old = LCD_SetFont(Display.listbox.font ? Display.listbox.font : DEFAULT_FONT.font);
    for(i = 0; i < listbox->entries_per_page; i++) {
        const char *str = listbox->string_cb(i + listbox->cur_pos, listbox->cb_data);
        LCD_SetFontColor(i + listbox->cur_pos == listbox->selected ? SELECT_TXT : TEXT);
       
        LCD_PrintString(str);
        LCD_PrintString("\n");
    }
    LCD_SetFont(old);
}

u8 GUI_TouchListbox(struct guiObject *obj, struct touch *coords, u8 long_press)
{
    struct guiListbox *listbox = &obj->o.listbox;
    struct guiBox box;
    u8 i;
    box.x = obj->box.x + obj->box.width - ARROW_WIDTH;
    box.y = obj->box.y;
    box.width = ARROW_WIDTH;
    box.height = ARROW_HEIGHT;
    if(coords_in_box(&box, coords)) {
        s16 new_pos = (s16)listbox->cur_pos - (long_press ? listbox->entries_per_page : 1);
        if (new_pos < 0)
            new_pos = 0;
        if(listbox->cur_pos != new_pos) {
            listbox->cur_pos = (u16)new_pos;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
        return 0;
    }
    box.y = obj->box.y + obj->box.height - ARROW_HEIGHT;
    if(coords_in_box(&box, coords)) {
        s16 new_pos = (s16)listbox->cur_pos + (long_press ? listbox->entries_per_page : 1);
        if (new_pos > listbox->item_count - listbox->entries_per_page)
            new_pos = listbox->item_count - listbox->entries_per_page;
        if(listbox->cur_pos != new_pos) {
            listbox->cur_pos = (u16)new_pos;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
        return 0;
    }
    box.x = obj->box.x;
    box.width = obj->box.width - ARROW_WIDTH;
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
