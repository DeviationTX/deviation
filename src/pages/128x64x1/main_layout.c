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

#if HAS_LAYOUT_EDITOR

static struct layout_page    * const lp  = &pagemem.u.layout_page;
static struct PageCfg2       * const pc  = &Model.pagecfg2;
static struct mainlayout_obj * const gui = &gui_objs.u.mainlayout;

#define NUMERIC_LABELS
static const int HEADER_Y = 1;

#include "../common/_main_layout.c"

static const char *pos_cb(guiObject_t *obj, const void *data);

static u8 _layaction_cb(u32 button, u8 flags, void *data);

static void move_cb(guiObject_t *obj, const void *data)
{
    //This will draw a little arrow bmp
    (void)data;
    const u8 bmp[] = {
        0b00000100,
        0b00001110,
        0b00000000,
        0b00001010,
        0b00011011,
        0b00001010,
        0b00000000,
        0b00001110,
        0b00000100,
    };
    for(unsigned i = 0; i < sizeof(bmp); i++) {
        for(int j = 0; j < 5; j++) {
            if(bmp[i] & (1 << j)) 
                LCD_DrawPixelXY(obj->box.x+i, obj->box.y+j, 0xffff);
        }
    }
}

void show_layout()
{
    GUI_RemoveAllObjects();
    PAGE_SetActionCB(_layaction_cb);
    lp->selected_x = 0;
    lp->selected_y = 0;
    for (int i = 0 ; i < 5; i++)
        gui->desc[i] = (struct LabelDesc){
            .font = MICRO_FONT.font,
            .font_color = 0xffff,
            .fill_color = 0x0000,
            .outline_color = 0xffff,
            .style = LABEL_SQUAREBOX,
        };
    gui->desc[1].style = LABEL_BRACKET; //Special case for trims

    struct LabelDesc micro = MICRO_FONT;
    struct LabelDesc rect = MICRO_FONT;
    micro.style = LABEL_LEFT;
    rect.fill_color = 0x0000;
    rect.outline_color = 0x0000;
    GUI_CreateRect(&gui->editelem, 41, 1, 9, 5, &rect);
    gui->editelem.CallBack = move_cb;
    GUI_CreateLabel(&gui->xlbl, 0,  1, NULL, micro, "X:");
    GUI_CreateLabelBox(&gui->x, 8, 1, 13, 6, &micro, pos_cb, NULL, (void *) 0L);
    GUI_CreateLabel(&gui->ylbl, 22, 1, NULL, micro, "Y:");
    GUI_CreateLabelBox(&gui->y, 30, 1, 9, 6, &micro, pos_cb, NULL, (void *) 1L);
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
    if (lp->selected_for_move >= 0) {
        int x = GUI_TextSelectHelper(lp->selected_x, 0, LCD_WIDTH-lp->selected_w, dir, 1, 10, NULL);
        if (x != lp->selected_x) {
            lp->selected_x = x;
            move_elem();
        }
    }
}

void ypos_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (lp->selected_for_move >= 0) {
        int y = GUI_TextSelectHelper(lp->selected_y, HEADER_Y, LCD_HEIGHT-lp->selected_h, dir, 1, 10, NULL);
        if (y != lp->selected_y) {
            lp->selected_y = y;
            move_elem();
        }
    }
}

void set_selected_for_move(int idx)
{
    lp->selected_for_move = idx;
    GUI_SetHidden((guiObject_t *)&gui->editelem, idx >= 0 ? 0 : 1);
}

const char *pos_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    sprintf(tempstring, "%d", data ? lp->selected_y : lp->selected_x);
    return tempstring;
}

void select_for_move(guiLabel_t *obj)
{
    GUI_SetSelected((guiObject_t *)obj);
    notify_cb((guiObject_t *)obj);
    int idx = guielem_idx((guiObject_t *)obj);
    if (lp->selected_for_move == idx)
        return;
    if (lp->selected_for_move >= 0) {
        GUI_Redraw((guiObject_t *)&gui->elem[lp->selected_for_move]);
    }
    set_selected_for_move(idx);
}

    
static u8 _layaction_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if(CHAN_ButtonIsPressed(button, BUT_EXIT) && !(flags & BUTTON_RELEASE)) {
        if (lp->selected_for_move >= 0) {
            set_selected_for_move(-1);
        } else {
            layout_exit();
            show_config();
        }
        return 1;
    }
    if (! GUI_GetSelected() || flags & BUTTON_RELEASE)
        return 0;
    if (CHAN_ButtonIsPressed(button, BUT_ENTER) && lp->selected_for_move < 0) {
        select_for_move((guiLabel_t *)GUI_GetSelected());
        return 1;
    }
    if (lp->selected_for_move < 0)
        return 0;
    if(CHAN_ButtonIsPressed(button, BUT_LEFT)) {
        xpos_cb(NULL, (flags & BUTTON_LONGPRESS) ? -2 : -1, NULL);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
        xpos_cb(NULL, (flags & BUTTON_LONGPRESS) ? 2 : 1, NULL);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_UP)) {
        ypos_cb(NULL, (flags & BUTTON_LONGPRESS) ? -2 : -1, NULL);
        return 1;
    }
    if(CHAN_ButtonIsPressed(button, BUT_DOWN)) {
        ypos_cb(NULL, (flags & BUTTON_LONGPRESS) ? 2 : 1, NULL);
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
