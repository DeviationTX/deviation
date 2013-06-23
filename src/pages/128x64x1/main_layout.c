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
#include "telemetry.h"

#if ENABLE_LAYOUT_EDIT

#define gui (&gui_objs.u.mainlayout)
#define pc Model.pagecfg2

#define NUMERIC_LABELS
#define HEADER_Y 10
#include "../common/_main_layout.c"

static const char *pos_cb(guiObject_t *obj, const void *data);

static u8 _layaction_cb(u32 button, u8 flags, void *data);

void show_layout()
{
    GUI_RemoveAllObjects();
    PAGE_SetActionCB(_layaction_cb);
    selected_x = 0;
    selected_y = 0;
    for (int i = 0 ; i < 5; i++)
        gui->desc[i] = (struct LabelDesc){
            .font = MICRO_FONT.font,
            .font_color = 0xffff,
            .fill_color = 0x0000,
            .outline_color = 0xffff,
            .style = LABEL_SQUAREBOX,
        };
    gui->desc[1].style = LABEL_BRACKET; //Special case for trims

    GUI_CreateLabelBox(&gui->editelem, LCD_WIDTH-30, 1, 29, 10, &SMALL_FONT, NULL, NULL, "Move");
    GUI_CreateLabel(&gui->xlbl, 0, 1, NULL, SMALL_FONT, "X:");
    GUI_CreateLabelBox(&gui->x, 10, 1, 20, 10, &SMALL_FONT, pos_cb, NULL, (void *) 0L);
    GUI_CreateLabel(&gui->ylbl, 32, 1, NULL, SMALL_FONT, "Y:");
    GUI_CreateLabelBox(&gui->y, 42, 1, 20, 10, &SMALL_FONT, pos_cb, NULL, (void *) 1L);
    //gui->y must be the last element!
    GUI_SelectionNotify(notify_cb);

    draw_elements();
    if(OBJ_IS_USED(&gui->elem[0]))
        GUI_SetSelected((guiObject_t *)&gui->elem[0]);
}
void layout_exit() 
{
    GUI_SelectionNotify(NULL);
    PAGE_SetActionCB(NULL);
}

void xpos_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (selected_for_move) {
        int x = GUI_TextSelectHelper(selected_x, 0, LCD_WIDTH-selected_w, dir, 1, 10, NULL);
        if (x != selected_x) {
            selected_x = x;
            move_elem();
        }
    }
}

void ypos_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (selected_for_move) {
        int y = GUI_TextSelectHelper(selected_y, 10, LCD_HEIGHT-selected_h, dir, 1, 10, NULL);
        if (y != selected_y) {
            selected_y = y;
            move_elem();
        }
    }
}

void set_selected_for_move(int idx)
{
    selected_for_move = idx >= 0 ? &gui->elem[idx] : NULL;
    GUI_SetHidden((guiObject_t *)&gui->editelem, idx >= 0 ? 0 : 1);
}

const char *pos_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    sprintf(tmp, "%d", data ? selected_y : selected_x);
    return tmp;
}

void select_for_move(guiLabel_t *obj)
{
    GUI_SetSelected((guiObject_t *)obj);
    notify_cb((guiObject_t *)obj);
    if (selected_for_move == obj)
        return;
    if (selected_for_move) {
        GUI_Redraw((guiObject_t *)selected_for_move);
    }
    set_selected_for_move(obj_to_idx(obj));
}

    
static u8 _layaction_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if(CHAN_ButtonIsPressed(button, BUT_EXIT) && !(flags & BUTTON_RELEASE)) {
        if (selected_for_move) {
            set_selected_for_move(-1);
        } else {
            show_config();
        }
        return 1;
    }
    if (! GUI_GetSelected() || flags & BUTTON_RELEASE)
        return 0;
    if (CHAN_ButtonIsPressed(button, BUT_ENTER) && ! selected_for_move) {
        select_for_move((guiLabel_t *)GUI_GetSelected());
        return 1;
    }
    if (! selected_for_move)
        return 0;
    if(CHAN_ButtonIsPressed(button, BUT_LEFT)) {
        xpos_cb(NULL, (flags & BUTTON_LONGPRESS) ? -2 : -1, (void *)selected_for_move->cb_data);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
        xpos_cb(NULL, (flags & BUTTON_LONGPRESS) ? 2 : 1, (void *)selected_for_move->cb_data);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_UP)) {
        ypos_cb(NULL, (flags & BUTTON_LONGPRESS) ? -2 : -1, (void *)selected_for_move->cb_data);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_DOWN)) {
        ypos_cb(NULL, (flags & BUTTON_LONGPRESS) ? 2 : 1, (void *)selected_for_move->cb_data);
        return 1;
    }
    return 0;
}
#else
void set_selected_for_move(int idx)
{
    (void)idx;
}
#endif
