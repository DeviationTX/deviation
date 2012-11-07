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
//#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "main_config.h"
#include "telemetry.h"

char str[30];
u8 page_num;

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

const struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} box_pos[8] = {
    { BOX0123_X, BOX04_Y, BOX_W, BOX0145_H },
    { BOX0123_X, BOX15_Y, BOX_W, BOX0145_H },
    { BOX0123_X, BOX26_Y, BOX_W, BOX2367_H },
    { BOX0123_X, BOX37_Y, BOX_W, BOX2367_H },
    { BOX4567_X, BOX04_Y, BOX_W, BOX0145_H },
    { BOX4567_X, BOX15_Y, BOX_W, BOX0145_H },
    { BOX4567_X, BOX26_Y, BOX_W, BOX2367_H },
    { BOX4567_X, BOX37_Y, BOX_W, BOX2367_H },
};

static void build_image();
static void show_page();
static void select_toggle_icon(u8 idx);

const char *trimsel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Model.pagecfg.trims = GUI_TextSelectHelper(Model.pagecfg.trims, 0, 3, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    switch(Model.pagecfg.trims) {
    case TRIMS_NONE :    return _tr("No Trims");
    case TRIMS_4OUTSIDE: return _tr("4 Outside");
    case TRIMS_4INSIDE:  return _tr("4 Inside");
    case TRIMS_6:        return _tr("6 Trims");
    default:             return "";
    }
}

const char *graphsel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    u8 maxbar;
    if ((Model.pagecfg.box[2] || Model.pagecfg.box[3]) && (Model.pagecfg.box[6] || Model.pagecfg.box[7]))
        maxbar = 0;
    else if (Model.pagecfg.box[2] || Model.pagecfg.box[3] || Model.pagecfg.box[6] || Model.pagecfg.box[7])
        maxbar = 1;
    else
        maxbar = 2;
    Model.pagecfg.barsize = GUI_TextSelectHelper(Model.pagecfg.barsize, 0, maxbar, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    switch(Model.pagecfg.barsize) {
    case BARS_NONE: return _tr("No Bars");
    case BARS_4:    return _tr("4 Bars");
    case BARS_8:    return _tr("8 Bars");
    default:        return "";
    }
}

const char *boxlabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 i = (long)data;
    sprintf(str, _tr("Box %d:"), i+1);
    return str;
}

const char *boxtxtsel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 i = (long)data;
    u8 changed;
    int old_val = Model.pagecfg.box[i];
    Model.pagecfg.box[i] = GUI_TextSelectHelper(Model.pagecfg.box[i], 0, NUM_CHANNELS + 2, dir, 1, 1, &changed);   
    if (changed && (old_val == 0 || Model.pagecfg.box[i] == 0))
        build_image();
    if (Model.pagecfg.box[i]) {
        if (Model.pagecfg.box[i] <= NUM_TIMERS)
            return TIMER_Name(str, Model.pagecfg.box[i] - 1);
        else if( Model.pagecfg.box[i] - NUM_TIMERS <= NUM_TELEM)
            return TELEMETRY_Name(str, Model.pagecfg.box[i] - NUM_TIMERS);
    }
    return INPUT_SourceName(str, Model.pagecfg.box[i] ? Model.pagecfg.box[i] - (NUM_TELEM + NUM_TIMERS) + NUM_INPUTS : 0);
}
const char *barlabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 i = (long)data;
    sprintf(str, _tr("Bar %d:"), i+1);
    return str;
}

const char *bartxtsel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 i = (long)data;
    u8 changed;
    Model.pagecfg.bar[i] = GUI_TextSelectHelper(Model.pagecfg.bar[i], 0, NUM_CHANNELS, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    return INPUT_SourceName(str, Model.pagecfg.bar[i] ? Model.pagecfg.bar[i] + NUM_INPUTS : 0);
}
const char *toggle_sel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 i = (long)data;
    sprintf(str, _tr("Toggle%d"), i+1);
    return str;
}

const char *toggle_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 i = (long)data;
    u8 changed;
    u8 val = MIXER_SRC(Model.pagecfg.toggle[i]);
    val = GUI_TextSelectHelper(val, 0, NUM_SOURCES, dir, 1, 1, &changed);   
    if (changed) {
        Model.pagecfg.toggle[i] = MIXER_SRC_IS_INV(Model.pagecfg.toggle[i]) | val;
        build_image();
    }
    GUI_TextSelectEnablePress(obj, MIXER_SRC(Model.pagecfg.toggle[i]));
    return INPUT_SourceName(str, Model.pagecfg.toggle[i]);
}

u8 MAINPAGE_GetWidgetLoc(enum MainWidget widget, u16 *x, u16 *y, u16 *w, u16 *h)
{
    u8 i;
    switch(widget) {
    case TRIM1:
    case TRIM2:
        if (! Model.pagecfg.trims)
            return 0;
        *y = TRIM_12_Y;
        *w = VTRIM_W;
        *h = VTRIM_H;
        if (widget == TRIM1) {
            *x = Model.pagecfg.trims == TRIMS_4OUTSIDE ? OUTTRIM_1_X : INTRIM_1_X;
        } else {
            *x = Model.pagecfg.trims == TRIMS_4OUTSIDE ? OUTTRIM_2_X : INTRIM_2_X;
        }
        return 1;
    case TRIM3:
    case TRIM4:
        if (! Model.pagecfg.trims)
            return 0;
        *x = (widget == TRIM3) ? TRIM_3_X : TRIM_4_X;
        *y = TRIM_34_Y;
        *w = HTRIM_W;
        *h = HTRIM_H;
        return 1;
    case TRIM5:
    case TRIM6:
        if (Model.pagecfg.trims != TRIMS_6)
            return 0;
        *x = (widget == TRIM5) ? TRIM_5_X : TRIM_6_X;
        *y = TRIM_56_Y;
        *w = VTRIM_W;
        *h = VTRIM_H;
        return 1;
    case TOGGLE4:
        if (Model.pagecfg.trims == TRIMS_6)
            return 0;
        //Fall-through
    case TOGGLE1:
    case TOGGLE2:
    case TOGGLE3:
        *w = TOGGLE_W;
        *h = TOGGLE_H;
        if (Model.pagecfg.trims != TRIMS_6) {
            *x = TOGGLE_CNTR_X;
            *y = TOGGLE_CNTR_Y + (widget - TOGGLE1) * TOGGLE_CNTR_SPACE;
            return 1;
        }
        if (! Model.pagecfg.box[3] && (Model.pagecfg.box[2] || Model.pagecfg.barsize == 0)) {
            *x = TOGGLE_L_X + (widget - TOGGLE1) * TOGGLE_LR_SPACE;
            *y = TOGGLE_LR_Y;
            return 1;
        } else if (! Model.pagecfg.box[7] && (Model.pagecfg.box[6] || Model.pagecfg.barsize == 0 || (Model.pagecfg.barsize == 1 && !Model.pagecfg.box[2] && !Model.pagecfg.box[3]))) {
            *x = TOGGLE_R_X + (widget - TOGGLE1) * TOGGLE_LR_SPACE;
            *y = TOGGLE_LR_Y;
            return 1;
        }
        return 0;
    case BOX1:
    case BOX2:
    case BOX3:
    case BOX4:
    case BOX5:
    case BOX6:
    case BOX7:
    case BOX8:
        i = widget-BOX1;
        if(Model.pagecfg.box[i]) {
            *x = box_pos[i].x;
            if (Model.pagecfg.trims == TRIMS_4OUTSIDE)
                *x += i < 4 ? OUTTRIM_OFFSET : -OUTTRIM_OFFSET;
            *y = box_pos[i].y;
            *w = box_pos[i].w;
            *h = box_pos[i].h;
            return 1;
        }
        return 0;
    case MODEL_ICO:
        if (! Model.pagecfg.box[4] && ! Model.pagecfg.box[5]) {
            *x = MODEL_ICO_X;
            if (Model.pagecfg.trims == TRIMS_4OUTSIDE)
                *x -= OUTTRIM_OFFSET;
        } else if(! Model.pagecfg.box[0] && ! Model.pagecfg.box[1]) {
            *x = 320 - MODEL_ICO_X - MODEL_ICO_W;
            if (Model.pagecfg.trims == TRIMS_4OUTSIDE)
                *x += OUTTRIM_OFFSET;
        } else
            return 0;
        *y = MODEL_ICO_Y;
        *w = MODEL_ICO_W;
        *h = MODEL_ICO_H;
        return 1;
    case BAR1:
    case BAR2:
    case BAR3:
    case BAR4:
    case BAR5:
    case BAR6:
    case BAR7:
    case BAR8:
        if(Model.pagecfg.barsize == BARS_NONE)
            return 0;
        *y = GRAPH_Y;
        *w = GRAPH_W;
        *h = GRAPH_H;
        i = 0;
        if(! Model.pagecfg.box[2] && ! Model.pagecfg.box[3]) {
            i = 4;
            if (widget <= BAR4) {
                *x = GRAPH1_X + GRAPH_SPACE * (widget - BAR1);
                if (Model.pagecfg.trims <= 1)
                    *x += OUTTRIM_OFFSET;
                return 1;
            }
        }
        if((! Model.pagecfg.box[6] && ! Model.pagecfg.box[7]) && (Model.pagecfg.barsize == BARS_8 || Model.pagecfg.box[2] || Model.pagecfg.box[3])) {
            if (widget >= (u8)(BAR1 + i) && widget < (u8)(BAR1 + i + 4)) {
                *x = GRAPH2_X + GRAPH_SPACE * (widget - (BAR1+i));
                if (Model.pagecfg.trims <= 1)
                    *x -= OUTTRIM_OFFSET;
                return 1;
            }
        }
        return 0;
    }
    return 0;
}
static void draw_rect(enum MainWidget widget, const struct LabelDesc *desc)
{
    u16 x, y, w, h;
    if (MAINPAGE_GetWidgetLoc(widget, &x, &y, &w, &h)) {
        GUI_CreateRect(CALC_X(x), CALC_Y(y), CALC_W(w), CALC_H(h), desc);
    }
}
static void build_image()
{
    int i;
    if(imageObj)
       GUI_RemoveHierObjects(imageObj);
    imageObj = GUI_CreateRect(IMAGE_X, IMAGE_Y, CALC_W(320), CALC_H(240-32), &outline);
    for(i = TRIM1; i <= TRIM6; i++)
        draw_rect(i, &fill_black);
    for(i = TOGGLE1; i <= TOGGLE4; i++)
        draw_rect(i, &fill_black);
    for(i = BOX1; i <= BOX8; i++)
        draw_rect(i, &fill_white);
    draw_rect(MODEL_ICO, &fill_black);
    for(i = BAR1; i <= BAR8; i++)
        draw_rect(i, &fill_black);
}

#define MAX_PAGE 1
static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)pos;
    (void)parent;
    (void)data;
    s8 newpos = (s8)page_num + direction;
    if (newpos < 0)
        newpos = 0;
    else if (newpos > MAX_PAGE)
        newpos = MAX_PAGE;
    if (newpos != page_num) {
        page_num = newpos;
        show_page();
    }
    return page_num;
}
void iconpress_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if(Model.pagecfg.toggle[(long)data])
        select_toggle_icon((long)data);
}
void toggle_inv_cb(guiObject_t *obj, void *data)
{
    if(MIXER_SRC(Model.pagecfg.toggle[(long)data])) {
        Model.pagecfg.toggle[(long)data] ^= 0x80;
        GUI_Redraw(obj);
    }
}

static void show_page()
{
    long i;
    if (firstObj) {
        GUI_RemoveHierObjects(firstObj);
        firstObj = NULL;
        imageObj = NULL;
    }

    u16 y = 144;
    if (page_num == 0) {
        GUI_CreateLabel(COL1_VALUE, 40, NULL, DEFAULT_FONT, _tr("Trims:"));
        firstObj = GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, trimsel_cb, NULL);
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
    }
    build_image();
}
void PAGE_MainCfgInit(int page)
{
    PAGE_SetModal(0);
    imageObj = NULL;
    firstObj = NULL;

    page_num = page;
    PAGE_ShowHeader(_tr("Main Page Config"));
    guiObject_t *obj = GUI_CreateScrollbar(304, 32, 208, MAX_PAGE, NULL, scroll_cb, NULL);
    GUI_SetScrollbar(obj, page);
    show_page();
}
void PAGE_MainCfgEvent()
{
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

void select_toggle_icon(u8 idx)
{
    (void)tglico_cancel_cb;
    (void)idx; // not used page, to be refactored later on
    /* long pos = 0;
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
    } */
}

