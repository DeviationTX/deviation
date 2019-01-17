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
#include "buttons.h"
#include "config/display.h"

static unsigned button_cb(u32 button, unsigned flags, void *data);

#define PRESS_UP     0x01
#define RELEASE_UP   0x02
#define PRESS_DOWN   0x04
#define RELEASE_DOWN 0x08
#define LONGPRESS    0x80

#define BAR_HEIGHT   ((ARROW_UP == NULL) ? 6 : 10)

guiObject_t *GUI_CreateScrollbar(guiScrollbar_t *scrollbar, u16 x, u16 y, u16 height,
    u8 num_items, struct guiObject *parent,
    int (*press_cb)(struct guiObject *parent, u8 pos, s8 direction, void *data), void *data)
{
    struct guiObject    *obj = (guiObject_t *)scrollbar;
    struct guiBox     *box;
    CLEAR_OBJ(scrollbar);

    box = &obj->box;
    box->x = x;
    box->y = y;
    box->width = ARROW_WIDTH;
    box->height = height;

    obj->Type = Scrollbar;
    OBJ_SET_TRANSPARENT(obj, 0);
    connect_object(obj);

    scrollbar->callback = press_cb;
    scrollbar->cb_data = data;
    scrollbar->parent = parent;
    scrollbar->num_items = num_items;
    scrollbar->cur_pos = 0;
    scrollbar->state = RELEASE_UP | RELEASE_DOWN;
    if (ARROW_UP != NULL)  // don't catch key events for devo10
        BUTTON_RegisterCallback(&scrollbar->action, CHAN_ButtonMask(BUT_DOWN) | CHAN_ButtonMask(BUT_UP), BUTTON_LONGPRESS, button_cb, obj);

    return obj;
}

void GUI_DrawScrollbar(struct guiObject *obj)
{
    struct guiScrollbar *scrollbar = (struct guiScrollbar *)obj;
    u16 bar_height = BAR_HEIGHT;  // Bug fix: After the logic view is introduce , a coordinate can be larger than 10000
    if (scrollbar->num_items <= 1)
        return;
    if (ARROW_UP == NULL) { // plate-text scroll bar for devo10
        // Improve scroll bar apperance for devo10
        GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
        LCD_DrawRect(obj->box.x, obj->box.y, obj->box.width, obj->box.height, 0xffff);
        u16 bar_y = scrollbar->cur_pos * (obj->box.height- bar_height)/(scrollbar->num_items -1)+ obj->box.y;
        LCD_FillRect(obj->box.x, bar_y, obj->box.width, bar_height, 0xffff);
    } else {
        #define BAR_BG      Display.scrollbar.bg_color      // RGB888_to_RGB565(0x44, 0x44, 0x44)
        #define BAR_FG      Display.scrollbar.fg_color      // RGB888_to_RGB565(0xaa, 0xaa, 0xaa)
        GUI_DrawImageHelper(obj->box.x, obj->box.y, ARROW_UP, scrollbar->state & PRESS_UP ? DRAW_PRESSED : DRAW_NORMAL);
        GUI_DrawImageHelper(obj->box.x, obj->box.y + obj->box.height - ARROW_HEIGHT,
                            ARROW_DOWN, scrollbar->state & PRESS_DOWN ? DRAW_PRESSED : DRAW_NORMAL);
        u8 bar = scrollbar->cur_pos * (obj->box.height - 2 * ARROW_HEIGHT - bar_height) / (scrollbar->num_items -1);
        LCD_FillRect(obj->box.x, obj->box.y + ARROW_HEIGHT,
                     ARROW_WIDTH, obj->box.height - 2 * ARROW_HEIGHT, BAR_BG);
        LCD_FillRect(obj->box.x, obj->box.y + ARROW_HEIGHT + bar,
                     ARROW_WIDTH, bar_height, BAR_FG);
    }
}

void GUI_SetScrollbar(struct guiScrollbar *scrollbar, u8 pos)
{
    guiObject_t *obj = &scrollbar->header;
    if (scrollbar->cur_pos != pos) {
        scrollbar->cur_pos = pos;
        OBJ_SET_DIRTY(obj, 1);
    }
}

u8 GUI_TouchScrollbar(struct guiObject *obj, struct touch *coords, s8 press_type)
{
    struct guiScrollbar *scrollbar = (struct guiScrollbar *)obj;
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
            int pos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos, -1, scrollbar->cb_data);
            if (pos >= 0)
                scrollbar->cur_pos = pos;
            OBJ_SET_DIRTY(obj, 1); //Set dirty to clear button press even if pos dint' move
            scrollbar->state = RELEASE_UP;
            return 1;
        }
        if (scrollbar->state == PRESS_DOWN) {
            int pos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos, 1, scrollbar->cb_data);
            if (pos >= 0)
                scrollbar->cur_pos = pos;
            OBJ_SET_DIRTY(obj, 1); //Set dirty to clear button press even if pos dint' move
            scrollbar->state = RELEASE_DOWN;
            return 1;
        }
        return 0;
    }
    unsigned bar_pos = scrollbar->cur_pos * (obj->box.height - 2 * ARROW_HEIGHT - BAR_HEIGHT) / (scrollbar->num_items -1)
                       + BAR_HEIGHT / 2 + ARROW_HEIGHT;
    box.x = obj->box.x;
    box.y = obj->box.y;
    box.width = ARROW_WIDTH;
    box.height = bar_pos;
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
        box.y = obj->box.y + bar_pos;
        box.height = obj->box.height - bar_pos;
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
        int newpos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos, dir, scrollbar->cb_data);
        if (newpos >= 0 && newpos != scrollbar->cur_pos) {
            scrollbar->cur_pos = newpos;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
    }
    return 0;
}

u8 GUI_GetScrollbarNumItems(struct guiScrollbar *scrollbar)
{
    return scrollbar->num_items;
}

static unsigned button_cb(u32 button, unsigned flags, void *data)
{
    struct guiObject *obj = (struct guiObject *)data;
    struct guiScrollbar *scrollbar = (struct guiScrollbar *)obj;

    if (!(flags & BUTTON_LONGPRESS))
        return 0;
    s8 dir = (button & CHAN_ButtonMask(BUT_DOWN)) ? 2 : -2;
    int newpos = scrollbar->callback(scrollbar->parent,  scrollbar->cur_pos,dir, scrollbar->cb_data);
    if (newpos >= 0 && newpos != scrollbar->cur_pos) {
        scrollbar->cur_pos = newpos;
        OBJ_SET_DIRTY(obj, 1);
    }
    return 1;
}
