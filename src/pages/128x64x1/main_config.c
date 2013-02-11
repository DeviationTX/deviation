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
#include "../common/main_config.h"
#include "telemetry.h"

#define gui (&gui_objs.u.mainconfig)

enum {
    ITEM_TRIMS,
    ITEM_BOX0,
    ITEM_BOX1,
    ITEM_BOX2,
    ITEM_BOX3,
    ITEM_BOX4,
    ITEM_BOX5,
    ITEM_BOX6,
    ITEM_BOX7,
    ITEM_SWITCH0,
    ITEM_SWITCH1,
    ITEM_SWITCH2,
    ITEM_SWITCH3,
    ITEM_MENU,
};

#define COL1_VALUE 4
#define COL2_VALUE 56
#define COL3_VALUE 156
#define COL4_VALUE 204
const struct LabelDesc outline = {0, 0, 0, 0, LABEL_TRANSPARENT};
const struct LabelDesc fill_white = {0, 0, 0xFFFF, 0, LABEL_FILL};
const struct LabelDesc fill_black = {0, 0, 0, 0, LABEL_FILL};
guiObject_t *imageObj;
guiObject_t *firstObj;

#define IMAGE_X 163
#define IMAGE_Y 40
#define CALC_X(x) ((x) * 10 / 24 + IMAGE_X)
#define CALC_Y(y) (((y)-32) * 10 / 24 + IMAGE_Y)
#define CALC_W(x) ((x) * 10 / 24)
#define CALC_H(y) ((y) * 10 / 24)

/*************************************/
/* Trims */
#define OUTTRIM_OFFSET 5

#define VTRIM_W 4 //10
#define VTRIM_H 49 // 140
#define HTRIM_W 49 //125
#define HTRIM_H 4 //10

#define INTRIM_1_X 59 //130
#define INTRIM_2_X (LCD_WIDTH - INTRIM_1_X - VTRIM_W)
#define OUTTRIM_1_X 1 //16
#define OUTTRIM_2_X (LCD_WIDTH - OUTTRIM_1_X - VTRIM_W)


#define TRIM_12_Y 10 // 75

#define TRIM_3_X 5 //5
#define TRIM_4_X (LCD_WIDTH - TRIM_3_X - HTRIM_W)
#define TRIM_34_Y 59 //220

#define TRIM_5_X 53
#define TRIM_6_X (LCD_WIDTH - TRIM_5_X - VTRIM_W)
#define TRIM_56_Y 9
#define VTRIM_56_H (VTRIM_H - 2)
/*************************************/


#define BOX0123_X 2 //16
#define BOX4567_X 80 //204
#define BOX_W     48 //100
#define BOX0145_H 9 //40
#define BOX2367_H 9 //24

#define BOX04_Y 22 //40
#define BOX15_Y 31 //90
#define BOX26_Y 39 //150
#define BOX37_Y 49 //185

/*************************************/
/* Model Icon */
#define MODEL_ICO_X 75
#define MODEL_ICO_Y 20
#define MODEL_ICO_W 52
#define MODEL_ICO_H 36
/*************************************/

#define GRAPH1_X BOX0123_X
#define GRAPH2_X (320 - BOX_W - GRAPH1_X)
#define GRAPH_Y BOX26_Y
#define GRAPH_H 59
#define GRAPH_W 10
#define GRAPH_SPACE ((BOX_W - GRAPH_W) / 3)

/*************************************/
/* Toggle Icons */

#define TOGGLE_CNTR_X 145
#define TOGGLE_CNTR_Y 40
#define TOGGLE_CNTR_SPACE 48

#define TOGGLE_LR_Y 11
#define TOGGLE_LR_SPACE 40
#define TOGGLE_L_X 8
#define TOGGLE_R_X (LCD_WIDTH - TOGGLE_L_X - 2 * TOGGLE_LR_SPACE - TOGGLEICON_WIDTH)
/*************************************/

enum {
    MAIN_PAGE,
    SWITCHICON_SELECTION_PAGE
} page_type;

#include "../common/_main_config.c"

#define VIEW_ID 0

static u8 _action_cb(u32 button, u8 flags, void *data);

static u16 current_selected = 0;

static int size_cb(int absrow, void *data)
{
    (void)data;
    return (absrow >= ITEM_MENU) ? 2 : 1;
}

static void switchicon_press_cb(guiObject_t *obj, const void *data)
{
    page_type = SWITCHICON_SELECTION_PAGE;
    current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
    TGLICO_Select(obj, data);
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    if(col == 0 && gui->col1[relrow].label.header.Type == Button) {  // both gui->col1[relrow].button and gui->col1[relrow].lable are the same pointer
        return (guiObject_t *)&gui->col1[relrow];
    }
    return (guiObject_t *)&gui->value[relrow];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    const void *label;
    void *label_cb = NULL;

    void *label_press = NULL;
    void *tgl = NULL;
    void *buttonlabel_cb = NULL;
    void *buttonpress_cb = NULL;
    void *value = NULL;
    int x = 56;
    int y_ts = y;
    switch(absrow) {
        case ITEM_TRIMS:
            label = _tr("Trims:");
            value = trimsel_cb;
            buttonlabel_cb = NULL;
            break;
        case ITEM_BOX0:
        case ITEM_BOX1:
        case ITEM_BOX2:
        case ITEM_BOX3:
        case ITEM_BOX4:
        case ITEM_BOX5:
        case ITEM_BOX6:
        case ITEM_BOX7:
	    buttonlabel_cb = NULL;
            label_cb = boxlabel_cb; label = (void *)(long)(absrow - ITEM_BOX0);
            value = boxtxtsel_cb; data = (void *)(long)(absrow - ITEM_BOX0);
            break;
        case ITEM_SWITCH0:
        case ITEM_SWITCH1:
        case ITEM_SWITCH2:
        case ITEM_SWITCH3:
	    label_cb = NULL;
            value = toggle_val_cb; data = (void *)(long)(absrow - ITEM_SWITCH0);
	    buttonlabel_cb = toggle_sel_cb; buttonpress_cb = switchicon_press_cb;
            break;
        case ITEM_MENU:
        default:
	    buttonlabel_cb = NULL;
            label_cb = menulabel_cb; label = (void *)(long)(absrow - ITEM_MENU);
            x = 56; 
            value = menusel_cb; data = (void *)(long)(absrow - ITEM_MENU); x = 0; y_ts += ITEM_HEIGHT;
            break;
    }

     if (buttonlabel_cb)
        GUI_CreateButtonPlateText(&gui->col1[relrow].button, 0, y,  50,
                ITEM_HEIGHT, &DEFAULT_FONT, buttonlabel_cb, 0x0000, buttonpress_cb, data);
    else
        GUI_CreateLabelBox(&gui->col1[relrow].label, 0, y,  0, ITEM_HEIGHT, &DEFAULT_FONT, label_cb, label_press, label);
    if (value) {
        GUI_CreateTextSelectPlate(&gui->value[relrow], x, y_ts,
             LCD_WIDTH-x-4, ITEM_HEIGHT, &DEFAULT_FONT, tgl, value, data);
    }
    return 1;
}
static void _show_page()
{
    page_type = MAIN_PAGE;
    GUI_CreateScrollable(&gui->scrollable, 0, ITEM_HEIGHT + 1, LCD_WIDTH, LCD_HEIGHT - ITEM_HEIGHT -1,
                     ITEM_SPACE, ITEM_MENU + NUM_QUICKPAGES, row_cb, getobj_cb, size_cb, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}

static void _show_title()
{
    PAGE_SetActionCB(_action_cb);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("Preview: Long-Press ENT"));
}

u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
	    if (page_type == MAIN_PAGE)
                PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
            else
                PAGE_MainCfgInit(-1);
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER) &&(flags & BUTTON_LONGPRESS)) {
	    if (page_type == MAIN_PAGE)
                PAGE_ChangeByID(PAGEID_MAIN, 1);
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT) && page_type == SWITCHICON_SELECTION_PAGE) {
            guiObject_t *obj = GUI_GetSelected();
            GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
        }  else if (CHAN_ButtonIsPressed(button, BUT_LEFT) && page_type == SWITCHICON_SELECTION_PAGE) {
            guiObject_t *obj = GUI_GetSelected();
            GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
void PAGE_MainCfgExit()
{
    current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
}
void _update_preview() {}
