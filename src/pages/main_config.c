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
char str[20];

void build_image();

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
#define COL1_VALUE 5
#define COL2_VALUE 40
#define COL3_VALUE 140
#define COL4_VALUE 175
const struct LabelDesc outline = {0, 0, 0, 0, TRANSPARENT};
const struct LabelDesc fill_white = {0, 0, 0xFFFF, 0, FILL};
const struct LabelDesc fill_black = {0, 0, 0, 0, FILL};
guiObject_t *imageObj;
guiObject_t *boxObj[8];
guiObject_t *boxlblObj[8];

#define IMAGE_X 177
#define IMAGE_Y 40
#define CALC_X(x) ((x) * 10 / 24 + IMAGE_X)
#define CALC_Y(y) ((y) * 10 / 24 + IMAGE_Y)
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
void build_image()
{
    int i;
    if(imageObj)
       GUI_RemoveHierObjects(imageObj);
    imageObj = GUI_CreateRect(IMAGE_X, IMAGE_Y, 133, 100, &outline);

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
void PAGE_MainCfgInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    imageObj = 0;

    trims = 3;
    box[0] = 1;
    box[1] = 1;
    box[2] = 1;
    box[3] = 1;
    barsize = 1;

    u16 y = 144;
    long i;
    GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, trimsel_cb, NULL);
    GUI_CreateTextSelect(COL1_VALUE, 64, TEXTSELECT_96, 0x0000, NULL, graphsel_cb, NULL);
    for(i = 0; i < 4; i++) {
        GUI_CreateLabel(COL1_VALUE, y, boxlabel_cb, DEFAULT_FONT, (void *)i);
        boxlblObj[i] = boxObj[i] = GUI_CreateTextSelect(COL2_VALUE, y, TEXTSELECT_96, 0x0000, NULL, boxtxtsel_cb, (void *)i);
        boxlblObj[4+i] = GUI_CreateLabel(COL3_VALUE, y, boxlabel_cb, DEFAULT_FONT, (void *)(4L+i));
        boxObj[4+i] = GUI_CreateTextSelect(COL4_VALUE, y, TEXTSELECT_96, 0x0000, NULL, boxtxtsel_cb, (void *)(4L+i));
        y+= 24;
    }
    build_image();
}
void PAGE_MainCfgEvent()
{
}
