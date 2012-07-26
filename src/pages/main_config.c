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
void build_image()
{
    int i;
    if(imageObj)
       GUI_RemoveHierObjects(imageObj);
    imageObj = GUI_CreateRect(IMAGE_X, IMAGE_Y, CALC_W(320), CALC_H(240-32), &outline);

    if (trims == TRIMS_4OUTSIDE) {
        GUI_CreateRect(CALC_X(OUTTRIM_1_X), CALC_Y(TRIM_12_Y), CALC_W(VTRIM_W), CALC_H(VTRIM_H), &fill_black);
        GUI_CreateRect(CALC_X(OUTTRIM_2_X), CALC_Y(TRIM_12_Y), CALC_W(VTRIM_W), CALC_H(VTRIM_H), &fill_black);
    } else if(trims == TRIMS_4INSIDE || trims == TRIMS_6) {
        GUI_CreateRect(CALC_X(INTRIM_1_X), CALC_Y(TRIM_12_Y), CALC_W(VTRIM_W), CALC_H(VTRIM_H), &fill_black);
        GUI_CreateRect(CALC_X(INTRIM_2_X), CALC_Y(TRIM_12_Y), CALC_W(VTRIM_W), CALC_H(VTRIM_H), &fill_black);
    }
    if (trims > 0) {
        GUI_CreateRect(CALC_X(TRIM_3_X), CALC_Y(TRIM_34_Y), CALC_W(HTRIM_W), CALC_H(HTRIM_H), &fill_black);
        GUI_CreateRect(CALC_X(TRIM_4_X), CALC_Y(TRIM_34_Y), CALC_W(HTRIM_W), CALC_H(HTRIM_H), &fill_black);
    }
    if (trims == TRIMS_6) {
        GUI_CreateRect(CALC_X(TRIM_5_X), CALC_Y(TRIM_56_Y), CALC_W(VTRIM_W), CALC_H(VTRIM_H), &fill_black);
        GUI_CreateRect(CALC_X(TRIM_6_X), CALC_Y(TRIM_56_Y), CALC_W(VTRIM_W), CALC_H(VTRIM_H), &fill_black);
        if (! box[3] && (barsize == 0 || (barsize == 1 && !box[6] && !box[7]))) {
            GUI_CreateRect(CALC_X(ICO_L_X + 0*ICO_LR_SPACE), CALC_Y(ICO_LR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
            GUI_CreateRect(CALC_X(ICO_L_X + 1*ICO_LR_SPACE), CALC_Y(ICO_LR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
            GUI_CreateRect(CALC_X(ICO_L_X + 2*ICO_LR_SPACE), CALC_Y(ICO_LR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
        } else if (! box[7] && (barsize == 0 || (barsize == 1 && !box[2] && !box[3]))) {
            GUI_CreateRect(CALC_X(320 - ICO_L_X - 2*ICO_LR_SPACE - ICO_W), CALC_Y(ICO_LR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
            GUI_CreateRect(CALC_X(320 - ICO_L_X - 1*ICO_LR_SPACE - ICO_W), CALC_Y(ICO_LR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
            GUI_CreateRect(CALC_X(320 - ICO_L_X - 0*ICO_LR_SPACE - ICO_W), CALC_Y(ICO_LR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
        } 
    } else {
        GUI_CreateRect(CALC_X(ICO_CNTR_X), CALC_Y(ICO1_CNTR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
        GUI_CreateRect(CALC_X(ICO_CNTR_X), CALC_Y(ICO2_CNTR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
        GUI_CreateRect(CALC_X(ICO_CNTR_X), CALC_Y(ICO3_CNTR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
        GUI_CreateRect(CALC_X(ICO_CNTR_X), CALC_Y(ICO4_CNTR_Y), CALC_W(ICO_W), CALC_H(ICO_H), &fill_black);
    }
    for (i = 0; i < 8; i++) {
        if(box[i]) {
            u16 x = box_pos[i].x;
            if (trims == TRIMS_4OUTSIDE)
                x += i < 4 ? 24 : -24;
            GUI_CreateRect(CALC_X(x), CALC_Y(box_pos[i].y), CALC_W(box_pos[i].w), CALC_H(box_pos[i].h), &fill_white);
        }
    }
    if (! box[4] && ! box[5])
        GUI_CreateRect(CALC_X(ICON_X), CALC_Y(ICON_Y), CALC_W(ICON_W), CALC_H(ICON_H), &fill_black);
    else if (! box[0] && ! box[1])
        GUI_CreateRect(CALC_X(320 - ICON_X - ICON_W), CALC_Y(ICON_Y), CALC_W(ICON_W), CALC_H(ICON_H), &fill_black);
    if (barsize) {
        if (barsize == 2 || (! box[2] && ! box[3])) {
            if (trims <= 1) {
                for(i = 0; i < 4; i++) {
                    GUI_CreateRect(CALC_X(24+24+30*i), CALC_Y(BOX26_Y), CALC_W(10), CALC_H(GRAPH_H), &fill_black);
                }
            } else {
                for(i = 0; i < 3; i++) {
                    GUI_CreateRect(CALC_X(24+30*i), CALC_Y(BOX26_Y), CALC_W(10), CALC_H(GRAPH_H), &fill_black);
                }
            }
        }
        if (barsize == 2 || (box[2] || box[3])) {
            if (trims <= 1) {
                for(i = 0; i < 4; i++) {
                    GUI_CreateRect(CALC_X(204-24+30*i), CALC_Y(BOX26_Y), CALC_W(10), CALC_H(GRAPH_H), &fill_black);
                }
            } else {
                for(i = 0; i < 3; i++) {
                    GUI_CreateRect(CALC_X(204+30*i), CALC_Y(BOX26_Y), CALC_W(10), CALC_H(GRAPH_H), &fill_black);
                }
            }
        }
    }
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
