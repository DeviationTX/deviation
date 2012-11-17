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
#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "../common/main_config.h"
#include "telemetry.h"

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
#define OUTTRIM_OFFSET 24

#define VTRIM_W 10
#define VTRIM_H 140
#define HTRIM_W 125
#define HTRIM_H 10

#define INTRIM_1_X 130
#define INTRIM_2_X (320 - INTRIM_1_X - VTRIM_W)
#define OUTTRIM_1_X 16
#define OUTTRIM_2_X (320 - OUTTRIM_1_X - VTRIM_W)
#define TRIM_12_Y 75

#define TRIM_3_X 5
#define TRIM_4_X (320 - TRIM_3_X - HTRIM_W)
#define TRIM_34_Y 220

#define TRIM_5_X 145
#define TRIM_6_X (320 - TRIM_5_X - VTRIM_W)
#define TRIM_56_Y 40
#define VTRIM_56_H VTRIM_H
/*************************************/


#define BOX0123_X 16
#define BOX4567_X 204
#define BOX_W     100
#define BOX0145_H 40
#define BOX2367_H 24

#define BOX04_Y 40
#define BOX15_Y 90
#define BOX26_Y 150
#define BOX37_Y 185

/*************************************/
/* Model Icon */
#define MODEL_ICO_X 205
#define MODEL_ICO_Y 40
#define MODEL_ICO_W 96
#define MODEL_ICO_H 96
/*************************************/

#define GRAPH1_X BOX0123_X
#define GRAPH2_X (320 - BOX_W - GRAPH1_X)
#define GRAPH_Y BOX26_Y
#define GRAPH_H 59
#define GRAPH_W 10
#define GRAPH_SPACE ((BOX_W - GRAPH_W) / 3)

/*************************************/
/* Toggle Icons */
#define TOGGLE_W 32
#define TOGGLE_H 32

#define TOGGLE_CNTR_X 145
#define TOGGLE_CNTR_Y 40
#define TOGGLE_CNTR_SPACE 48

#define TOGGLE_LR_Y 182
#define TOGGLE_LR_SPACE 40
#define TOGGLE_L_X 10
#define TOGGLE_R_X (320 - TOGGLE_L_X - 2 * TOGGLE_LR_SPACE - TOGGLE_W)
/*************************************/

#include "../common/_main_config.c"

static void select_toggle_icon(u8 idx);
static void iconpress_cb(guiObject_t *obj, const void *data);

#define MAX_PAGE 2 // bug fix: divided by 0 error; scroll bar is 1-based now
static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)pos;
    (void)parent;
    (void)data;
    s8 newpos = (s8)page_num + (direction > 0 ? 1 : -1);
    if (newpos < 0)
        newpos = 0;
    else if (newpos > MAX_PAGE)
        newpos = MAX_PAGE;
    if (newpos != page_num) {
        page_num = newpos;
        _show_page();
    }
    return page_num;
}
static void iconpress_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if(Model.pagecfg.toggle[(long)data])
        select_toggle_icon((long)data);
}

static void _show_page()
{
    long i;
    if (firstObj) {
        GUI_RemoveHierObjects(firstObj);
        firstObj = NULL;
        imageObj = NULL;
    }

    u16 y = 144;
    if (page_num == 0) {
        firstObj = GUI_CreateLabel(COL1_VALUE, 40, NULL, DEFAULT_FONT, _tr("Trims:"));
        GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, trimsel_cb, NULL);
        GUI_CreateLabel(COL1_VALUE, 64, NULL, DEFAULT_FONT, _tr("Bars:"));
        GUI_CreateTextSelect(COL2_VALUE, 64, TEXTSELECT_96, 0x0000, NULL, graphsel_cb, NULL);
        for(i = 0; i < 4; i++) {
            GUI_CreateLabel(COL1_VALUE, y, boxlabel_cb, DEFAULT_FONT, (void *)i);
            GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, boxtxtsel_cb, (void *)i);
            y+= 24;
        }
        y = 144;
        for(i = 4; i < 8; i++) {
            GUI_CreateLabel(COL3_VALUE, y, boxlabel_cb, DEFAULT_FONT, (void *)i);
            GUI_CreateTextSelect(COL4_VALUE, y, TEXTSELECT_96, 0x0000, NULL, boxtxtsel_cb, (void *)i);
            y+= 24;
        }
    } else if (page_num == 1) {
        firstObj = GUI_CreateButton(COL1_VALUE, 40, BUTTON_48x16, toggle_sel_cb, 0x0000, iconpress_cb, (void *)0);
        GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, toggle_inv_cb, toggle_val_cb, (void *)0);
        GUI_CreateButton(COL1_VALUE, 64, BUTTON_48x16, toggle_sel_cb, 0x0000, iconpress_cb, (void *)1);
        GUI_CreateTextSelect(COL2_VALUE, 64, TEXTSELECT_96, 0x0000, toggle_inv_cb, toggle_val_cb, (void *)1);
        GUI_CreateButton(COL1_VALUE, 88, BUTTON_48x16, toggle_sel_cb, 0x0000, iconpress_cb, (void *)2);
        GUI_CreateTextSelect(COL2_VALUE, 88, TEXTSELECT_96, 0x0000, toggle_inv_cb, toggle_val_cb, (void *)2);
        GUI_CreateButton(COL1_VALUE, 112, BUTTON_48x16, toggle_sel_cb, 0x0000, iconpress_cb, (void *)3);
        GUI_CreateTextSelect(COL2_VALUE, 112, TEXTSELECT_96, 0x0000, toggle_inv_cb, toggle_val_cb, (void *)3);
        for(i = 0; i < 4; i++) {
            GUI_CreateLabel(COL1_VALUE, y, barlabel_cb, DEFAULT_FONT, (void *)i);
            GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, bartxtsel_cb, (void *)i);
            y+= 24;
        }
        y = 144;
        for(i = 4; i < 8; i++) {
            GUI_CreateLabel(COL3_VALUE, y, barlabel_cb, DEFAULT_FONT, (void *)i);
            GUI_CreateTextSelect(COL4_VALUE, y, TEXTSELECT_96, 0x0000, NULL, bartxtsel_cb, (void *)i);
            y+= 24;
        }
    } else if (page_num == 2) {
        y = 40;
        firstObj = GUI_CreateLabel(COL1_VALUE, y, NULL, DEFAULT_FONT, _tr("Menu +"));
        GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, buttonsel_cb, (void *)0L);
        y += 24;
        GUI_CreateLabel(COL1_VALUE, y, NULL, DEFAULT_FONT, _tr("Menu -"));
        GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, buttonsel_cb, (void *)1L);
        y += 34;
        for (i = 0; i < 4; i++) {
            GUI_CreateLabel(COL1_VALUE, y, menulabel_cb, DEFAULT_FONT, (void *)i);
            GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, menusel_cb, (void *)i);
            y += 24;
        }
        return;
    }
    build_image();
}

static void _show_title()
{
     PAGE_ShowHeader(_tr("Main page config")); // using the same name as devo10's menu item to reduce language strings
     guiObject_t *obj = GUI_CreateScrollbar(304, 32, 208, MAX_PAGE+1, NULL, scroll_cb, NULL);
     GUI_SetScrollbar(obj, page_num);
}

void tglico_select_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        u16 pos = (long)data;
        u8 idx = pos >> 8;
        pos &= 0xff;
        Model.pagecfg.tglico[idx] = pos;
        PAGE_RemoveAllObjects();
        PAGE_MainCfgInit(1);
    }
}

static void tglico_cancel_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    PAGE_RemoveAllObjects();
    PAGE_MainCfgInit(1);
}
static void select_toggle_icon(u8 idx)
{
    long pos = 0;
    u16 w, h, x, y;
    u8 i, j;
    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    LCD_ImageDimensions(TOGGLE_FILE, &w, &h);
    u8 count = w / 32;
    u8 cursel = Model.pagecfg.tglico[idx];
    PAGE_CreateCancelButton(216, 4, tglico_cancel_cb);
    for(j = 0; j < 5; j++) {
        y = 40 + j * 40;
        for(i = 0; i < 8; i++,pos++) {
            if (pos >= count)
                break;
            x = 4 + i*40;
            if (pos == cursel)
                GUI_CreateRect(x-1, y-1, 34, 33, &outline);
            GUI_CreateImageOffset(x, y, 32, 31, pos * 32, 0, TOGGLE_FILE, tglico_select_cb, (void *)((idx << 8) | pos));
        }
    }
}
