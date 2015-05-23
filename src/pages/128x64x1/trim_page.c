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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"

enum {
    BUTTON_X      = 0,
    BUTTON_WIDTH  = 30,
    TEXTSEL_X     = 32,
    TEXTSEL_WIDTH = 50,
    LABEL_X       = 84,
    LABEL_WIDTH   = 41,
    STEP_X        = 46,
    STEP_Y        = 0,
    STEP_WIDTH    = 30,
    TRIMPOS_X     = 88,
    TRIMPOS_WIDTH = 30,
//
    LABEL2_X      = 0,
    LABEL2_WIDTH  = 0,
    TEXTSEL2_X    = 60,
    TEXTSEL2_WIDTH= 63,
//
    BUTTON2_X     = LCD_WIDTH - 50 - 1,
    BUTTON2_Y     = 0,
    BUTTON2_WIDTH = 50,
};
#endif //OVERRIDE_PLACEMENT

#include "../common/_trim_page.c"

static struct trim_obj  * const gui  = &gui_objs.u.trim;
static struct trim2_obj * const guit = &gui_objs.u.trim2;

static unsigned _action_cb(u32 button, unsigned flags, void *data);
static unsigned _sub_action_cb(u32 button, unsigned flags, void *data);
static u16 current_selected = 0;

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    col = (2 + col) % 2;
    return  (col == 0) ? (guiObject_t *)&gui->src[relrow] : (guiObject_t *)&gui->item[relrow];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    struct Trim *trim = MIXER_GetAllTrims();
    GUI_CreateButtonPlateText(&gui->src[relrow], BUTTON_X, y, BUTTON_WIDTH, LINE_HEIGHT,
            &DEFAULT_FONT, trimsource_name_cb, 0x0000, _edit_cb, (void *)((long)absrow));
    GUI_CreateTextSelectPlate(&gui->item[relrow], TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &TINY_FONT,  NULL, set_trimstep_cb, (void *)(long)(absrow+0x000)); //0x000: Use Model.trims
    GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT,
            &DEFAULT_FONT, NULL, NULL,  (void *)INPUT_ButtonName(trim[absrow].pos));
    return 2;
}

static void _show_page()
{
    PAGE_SetActionCB(_action_cb);
    //PAGE_ShowHeader(_tr("Trim")); // no title for devo10
    PAGE_ShowHeader(_tr("Input"));
    GUI_CreateLabelBox(&gui->steplbl, STEP_X, STEP_Y, STEP_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Step"));
    // no enought space in Devo10, so just display trim + in the 1st page
    //GUI_CreateLabelBox(w + 40, 0, 0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Trim -"));
    GUI_CreateLabelBox(&gui->trimposlbl, TRIMPOS_X, STEP_Y, TRIMPOS_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Trim +"));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, NUM_TRIMS, row_cb, getobj_cb, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}

void PAGE_TrimExit()
{
    current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
}

static guiObject_t *getobj2_cb(int relrow, int col, void *data)
{
    (void)data;
    (void)col;
    return (guiObject_t *)&guit->value[relrow];
}
enum {
    ITEM_INPUT,
    ITEM_TRIMSTEP,
    ITEM_TRIMNEG,
    ITEM_TRIMPOS,
    ITEM_TRIMSWITCH,
    ITEM_LAST,
};

static int row2_cb(int absrow, int relrow, int y, void *data)
{
    data = NULL;
    const void *label = "";
    void *value = NULL;

    switch(absrow) {
        case ITEM_INPUT:
            label = _tr_noop("Input");
            value = set_source_cb; data = &tp->trim.src;
            break;
        case ITEM_TRIMNEG:
            label = _tr_noop("Trim -");
            value = set_trim_cb; data = &tp->trim.neg;
            break;
        case ITEM_TRIMPOS:
            label = _tr_noop("Trim +");
            value = set_trim_cb; data = &tp->trim.pos;
            break;
        case ITEM_TRIMSTEP:
            label = _tr_noop("Trim Step");
            value = set_trimstep_cb; data = (void *)(long)(tp->index + 0x100); //0x100: Use tp->trim
            break;
        case ITEM_TRIMSWITCH:
            label = _tr_noop("Switch");
            value = set_switch_cb; data = &tp->trim.sw;
            break;
    }
    GUI_CreateLabelBox(&guit->label[relrow], LABEL2_X, y, LABEL2_WIDTH, LINE_HEIGHT,
            &DEFAULT_FONT, NULL, NULL,  _tr(label));
    GUI_CreateTextSelectPlate(&guit->value[relrow], TEXTSEL2_X, y,
            TEXTSEL2_WIDTH, LINE_HEIGHT, &DEFAULT_FONT,  NULL, value, data);
    return 1;
} 
static void _edit_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    PAGE_SetActionCB(_sub_action_cb);
    struct Trim *trim = MIXER_GetAllTrims();
    PAGE_SetModal(1);
    tp->index = (long)data;
    tp->trim = trim[tp->index];
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader("Edit"); // to draw a line only

    GUI_CreateButtonPlateText(&guit->save, BUTTON2_X, BUTTON2_Y, BUTTON2_WIDTH, LINE_HEIGHT,
            &DEFAULT_FONT, NULL, 0x0000, okcancel_cb, (void *)_tr("Save"));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, ITEM_LAST, row2_cb, getobj2_cb, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
}


static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static unsigned _sub_action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_TrimInit(0);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
static inline guiObject_t * _get_obj(int idx, int objid) {
    if (PAGE_GetModal()) {
        if(objid == TRIM_MINUS) {
            idx = ITEM_TRIMNEG; objid = -1;
        } else if(objid == TRIM_SWITCH) {
            idx = ITEM_TRIMSWITCH; objid = -1;
        }
        return (guiObject_t *)GUI_GetScrollableObj(&gui->scrollable, idx, objid);
    }
    return NULL;
}
