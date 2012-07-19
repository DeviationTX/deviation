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
#include "buttons.h"
#include "config/display.h"

static u8 button_cb(u32 button, u8 flags, void *data);

#define PRESS_UP     0x01
#define RELEASE_UP   0x02
#define PRESS_DOWN   0x04
#define RELEASE_DOWN 0x08
#define LONGPRESS    0x80

struct guiObject *GUI_CreateScrollbar(u16 x, u16 y, u16 height,
    u8 num_items, struct guiObject *parent,
    u8 (*press_cb)(struct guiObject *parent, u8 pos, s8 direction, void *data), void *data)
{
    struct guiObject    *obj = GUI_GetFreeObj();
    struct guiScrollbar *scrollbar;
    struct guiBox     *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    box->x = x;
    box->y = y;
    box->width = ARROW_WIDTH;
    box->height = height;

    obj->Type = Scrollbar;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    scrollbar = &obj->o.scrollbar;
    scrollbar->callback = press_cb;
    scrollbar->cb_data = data;
    scrollbar->parent = parent;
    scrollbar->num_items = num_items;
    scrollbar->cur_pos = 0;
    scrollbar->state = RELEASE_UP | RELEASE_DOWN;
    BUTTON_RegisterCallback(&scrollbar->action, CHAN_ButtonMask(BUT_DOWN) | CHAN_ButtonMask(BUT_UP), BUTTON_LONGPRESS, button_cb, obj);

    return obj;
}

void GUI_DrawScrollbar(struct guiObject *obj)
{
    #define BAR_HEIGHT 10
    #define BAR_BG      Display.listbox.bg_bar      // RGB888_to_RGB565(0x44, 0x44, 0x44)
    #define BAR_FG      Display.listbox.fg_bar      // RGB888_to_RGB565(0xaa, 0xaa, 0xaa)
    struct guiScrollbar *scrollbar = &obj->o.scrollbar;
    //if (scrollbar->state & (PRESS_UP | RELEASE_UP)) {
    GUI_DrawImageHelper(obj->box.x, obj->box.y, ARROW_UP, scrollbar->state & PRESS_UP ? DRAW_PRESSED : DRAW_NORMAL);
    //}
    //if (scrollbar->state & (PRESS_DOWN | RELEASE_DOWN)) {
    GUI_DrawImageHelper(obj->box.x, obj->box.y + obj->box.height - ARROW_HEIGHT,
                        ARROW_DOWN, scrollbar->state & PRESS_DOWN ? DRAW_PRESSED : DRAW_NORMAL);
    //}
    //if (! (scrollbar->state & (PRESS_UP | PRESS_DOWN))) {
        u8 bar = scrollbar->cur_pos * (obj->box.height - 2 * ARROW_HEIGHT - BAR_HEIGHT) / scrollbar->num_items;
        LCD_FillRect(obj->box.x, obj->box.y + ARROW_HEIGHT,
                     ARROW_WIDTH, obj->box.height - 2 * ARROW_HEIGHT, BAR_BG);
        LCD_FillRect(obj->box.x, obj->box.y + ARROW_HEIGHT + bar,
                     ARROW_WIDTH, BAR_HEIGHT, BAR_FG);
        //scrollbar->state = 0;
    //}
}

u8 GUI_TouchScrollbar(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    struct guiScrollbar *scrollbar = &obj->o.scrollbar;
    struct guiBox box;
    s8 dir = 0;

    if (press_type < 0) {
        if (!scrollbar->state)
            return 0;
        if (scrollbar->state & LONGPRESS) {
            scrollbar->state = (scrollbar->state & ~LONGPRESS) == PRESS_UP ? RELEASE_UP : RELEASE_DOWN;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
        if (scrollbar->state == PRESS_UP) {
            scrollbar->cur_pos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos, -1, scrollbar->cb_data);
            OBJ_SET_DIRTY(obj, 1);
            scrollbar->state = RELEASE_UP;
            return 1;
        }
        if (scrollbar->state == PRESS_DOWN) {
            scrollbar->cur_pos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos, 1, scrollbar->cb_data);
            OBJ_SET_DIRTY(obj, 1);
            scrollbar->state = RELEASE_DOWN;
            return 1;
        }
        return 0;
    }
    box.x = obj->box.x;
    box.y = obj->box.y;
    box.width = ARROW_WIDTH;
    box.height = ARROW_HEIGHT;
    if(coords_in_box(&box, coords)) {
        if(press_type) {
            dir = -2;
            scrollbar->state |= LONGPRESS;
        } else {
            scrollbar->state = PRESS_UP;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
    } else {
        box.y = obj->box.y + obj->box.height - ARROW_HEIGHT;
        if(coords_in_box(&box, coords)) {
            if(press_type) {
                dir = 2;
                scrollbar->state |= LONGPRESS;
            } else {
                scrollbar->state = PRESS_DOWN;
                OBJ_SET_DIRTY(obj, 1);
                return 1;
            }
        }
    }
    if(dir) {
        u8 newpos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos, dir, scrollbar->cb_data);
        if (newpos != scrollbar->cur_pos) {
            scrollbar->cur_pos = newpos;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
    }
    return 0;
}

static u8 button_cb(u32 button, u8 flags, void *data)
{
    struct guiObject *obj = (struct guiObject *)data;
    struct guiScrollbar *scrollbar = &obj->o.scrollbar;

    if (!(flags & BUTTON_LONGPRESS))
        return 0;
    s8 dir = (button & CHAN_ButtonMask(BUT_DOWN)) ? 2 : -2;
    u8 newpos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos,dir, scrollbar->cb_data);
    if (newpos != scrollbar->cur_pos) {
        scrollbar->cur_pos = newpos;
        OBJ_SET_DIRTY(obj, 1);
    }
    return 1;
}
