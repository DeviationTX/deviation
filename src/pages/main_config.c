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
#include "pages.h"
#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "main_config.h"

#define TRIMS_4OUTSIDE 1
#define TRIMS_4INSIDE  2
#define TRIMS_6        3
u8 trims;
u8 barsize;
u8 box[8];
u8 bar[8];
u8 icon[4];
char str[20];
u8 page_num;

#define COL1_VALUE 5
#define COL2_VALUE 45
#define COL3_VALUE 162
#define COL4_VALUE 202
const struct LabelDesc outline = {0, 0, 0, 0, TRANSPARENT};
const struct LabelDesc fill_white = {0, 0, 0xFFFF, 0, FILL};
const struct LabelDesc fill_black = {0, 0, 0, 0, FILL};
guiObject_t *imageObj;
guiObject_t *firstObj;

#define IMAGE_X 157
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

const char *trimsel_cb(guiObject_t *obj, int dir, void *data)
{
    u8 changed;
    trims = GUI_TextSelectHelper(trims, 0, 3, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    switch(trims) {
    case 0 : return "NoTrim";
    case 1 : return "OutTrim";
    case 2 : return "InTrim";
    case 3 : return "6Trim";
    default: return "";
    }
}

const char *graphsel_cb(guiObject_t *obj, int dir, void *data)
{
    u8 changed;
    u8 maxbar;
    if ((box[2] || box[3]) && (box[6] || box[7]))
        maxbar = 0;
    else if (box[2] || box[3] || box[6] || box[7])
        maxbar = 1;
    else
        maxbar = 2;
    barsize = GUI_TextSelectHelper(barsize, 0, maxbar, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    switch(barsize) {
    case 0 : return "NoGraphs";
    case 1 : return "HalfGraphs";
    case 2 : return "FullGraphs";
    default: return "";
    }
}

const char *boxlabel_cb(guiObject_t *obj, void *data)
{
    u8 i = (long)data;
    sprintf(str, "Box %d:", i);
    return str;
}

const char *boxtxtsel_cb(guiObject_t *obj, int dir, void *data)
{
    u8 i = (long)data;
    u8 changed;
    box[i] = GUI_TextSelectHelper(box[i], 0, 1, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    return box[i] ? "Enable" : "Disable";
}
const char *barlabel_cb(guiObject_t *obj, void *data)
{
    u8 i = (long)data;
    sprintf(str, "Bar %d:", i);
    return str;
}

const char *bartxtsel_cb(guiObject_t *obj, int dir, void *data)
{
    u8 i = (long)data;
    u8 changed;
    bar[i] = GUI_TextSelectHelper(bar[i], 0, 1, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    return bar[i] ? "Enable" : "Disable";
}
const char *iconlabel_cb(guiObject_t *obj, void *data)
{
    u8 i = (long)data;
    sprintf(str, "Icon %d:", i);
    return str;
}

const char *iconsel_cb(guiObject_t *obj, int dir, void *data)
{
    u8 i = (long)data;
    u8 changed;
    icon[i] = GUI_TextSelectHelper(icon[i], 0, 1, dir, 1, 1, &changed);   
    if (changed)
        build_image();
    return icon[i] ? "Enable" : "Disable";
}

u8 MAINPAGE_GetWidgetLoc(enum MainWidget widget, u16 *x, u16 *y, u16 *w, u16 *h)
{
    u8 i;
    switch(widget) {
    case TRIM1:
    case TRIM2:
        if (! trims)
            return 0;
        *y = TRIM_12_Y;
        *w = VTRIM_W;
        *h = VTRIM_H;
        if (widget == TRIM1) {
            *x = trims == TRIMS_4OUTSIDE ? OUTTRIM_1_X : INTRIM_1_X;
        } else {
            *x = trims == TRIMS_4OUTSIDE ? OUTTRIM_2_X : INTRIM_2_X;
        }
        return 1;
    case TRIM3:
    case TRIM4:
        if (! trims)
            return 0;
        *x = (widget == TRIM3) ? TRIM_3_X : TRIM_4_X;
        *y = TRIM_34_Y;
        *w = HTRIM_W;
        *h = HTRIM_H;
        return 1;
    case TRIM5:
    case TRIM6:
        if (trims != TRIMS_6)
            return 0;
        *x = (widget == TRIM5) ? TRIM_5_X : TRIM_6_X;
        *y = TRIM_56_Y;
        *w = VTRIM_W;
        *h = VTRIM_H;
        return 1;
    case TOGGLE4:
        if (trims == TRIMS_6)
            return 0;
        //Fall-through
    case TOGGLE1:
    case TOGGLE2:
    case TOGGLE3:
        *w = TOGGLE_W;
        *h = TOGGLE_H;
        if (trims != TRIMS_6) {
            *x = TOGGLE_CNTR_X;
            *y = TOGGLE_CNTR_Y + (widget - TOGGLE1) * TOGGLE_CNTR_SPACE;
            return 1;
        }
        if (! box[3] && (box[2] || barsize == 0)) {
            *x = TOGGLE_L_X + (widget - TOGGLE1) * TOGGLE_LR_SPACE;
            *y = TOGGLE_LR_Y;
            return 1;
        } else if (! box[7] && (box[6] || barsize == 0 || (barsize == 1 && !box[2] && !box[3]))) {
            *x = TOGGLE_R_X + (widget - TOGGLE1) * TOGGLE_LR_SPACE;
            *y = TOGGLE_LR_Y;
            return 1;
        }
    case BOX1:
    case BOX2:
    case BOX3:
    case BOX4:
    case BOX5:
    case BOX6:
    case BOX7:
    case BOX8:
        i = widget-BOX1;
        if(box[i]) {
            *x = box_pos[i].x;
            if (trims == TRIMS_4OUTSIDE)
                *x += i < 4 ? 24 : -24;
            *y = box_pos[i].y;
            *w = box_pos[i].w;
            *h = box_pos[i].h;
            return 1;
        }
    case MODEL_ICO:
        if (! box[4] && ! box[5]) {
            *x = MODEL_ICO_X;
            if (trims == TRIMS_4OUTSIDE)
                *x -= 24;
        } else if(! box[0] && ! box[1]) {
            *x = 320 - MODEL_ICO_X - MODEL_ICO_W;
            if (trims == TRIMS_4OUTSIDE)
                *x += 24;
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
        if(! barsize)
            return 0;
        *y = BOX26_Y;
        *w = 10;
        *h = GRAPH_H;
        i = 0;
        if(! box[2] && ! box[3]) {
            if (trims <= 1) {
                i = 4;
                if (widget <= BAR4) {
                    *x = 24+24+30*(widget - BAR1);
                    return 1;
                }
            } else {
                i = 3;
                if (widget <= BAR3) {
                    *x = 24+30*(widget - BAR1);
                    return 1;
                }
            }
        }
        if((! box[6] && ! box[7]) && (barsize == 2 || box[2] || box[3])) {
            if (trims <= 1) {
                if (widget >= (u8)(BAR1 + i) && widget < (u8)(BAR1 + i + 4)) {
                    *x = 320 - 24 - 24 - 3*30 + 30 *(widget - (BAR1+i));
                    return 1;
                }
            } else {
                if (widget >= (u8)(BAR1 + i) && widget < (u8)(BAR1 + i + 3)) {
                    *x = 320 - 24 - 3*30 + 30 *(widget - (BAR1+i));
                    return 1;
                }
            }
        }
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
        firstObj = GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, trimsel_cb, NULL);
        GUI_CreateTextSelect(COL1_VALUE, 64, TEXTSELECT_96, 0x0000, NULL, graphsel_cb, NULL);
        for(i = 0; i < 4; i++) {
            GUI_CreateLabel(COL1_VALUE, y, boxlabel_cb, DEFAULT_FONT, (void *)i);
            GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, boxtxtsel_cb, (void *)i);
            GUI_CreateLabel(COL3_VALUE, y, boxlabel_cb, DEFAULT_FONT, (void *)(4L+i));
            GUI_CreateTextSelect(COL4_VALUE, y, TEXTSELECT_96, 0x0000, NULL, boxtxtsel_cb, (void *)(4L+i));
            y+= 24;
        }
    } else if (page_num == 1) {
        firstObj = GUI_CreateLabel(COL1_VALUE, 40, iconlabel_cb, DEFAULT_FONT, (void *)0);
        GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, iconsel_cb, (void *)0);
        GUI_CreateLabel(COL1_VALUE, 64, iconlabel_cb, DEFAULT_FONT, (void *)1);
        GUI_CreateTextSelect(COL2_VALUE, 64, TEXTSELECT_96, 0x0000, NULL, iconsel_cb, (void *)1);
        GUI_CreateLabel(COL1_VALUE, 88, iconlabel_cb, DEFAULT_FONT, (void *)2);
        GUI_CreateTextSelect(COL2_VALUE, 88, TEXTSELECT_96, 0x0000, NULL, iconsel_cb, (void *)2);
        GUI_CreateLabel(COL1_VALUE, 112, iconlabel_cb, DEFAULT_FONT, (void *)3);
        GUI_CreateTextSelect(COL2_VALUE, 112, TEXTSELECT_96, 0x0000, NULL, iconsel_cb, (void *)3);
        for(i = 0; i < 4; i++) {
            GUI_CreateLabel(COL1_VALUE, y, barlabel_cb, DEFAULT_FONT, (void *)i);
            GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, bartxtsel_cb, (void *)i);
            GUI_CreateLabel(COL3_VALUE, y, barlabel_cb, DEFAULT_FONT, (void *)(4L+i));
            GUI_CreateTextSelect(COL4_VALUE, y, TEXTSELECT_96, 0x0000, NULL, bartxtsel_cb, (void *)(4L+i));
            y+= 24;
        }
    }
    build_image();
}
void PAGE_MainCfgInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    imageObj = NULL;
    firstObj = NULL;

    trims = 3;
    box[0] = 1;
    box[1] = 1;
    box[2] = 1;
    box[3] = 1;
    barsize = 1;

    page_num = 0;
    GUI_CreateScrollbar(304, 32, 208, MAX_PAGE, NULL, scroll_cb, NULL);
    show_page();
}
void PAGE_MainCfgEvent()
{
}
