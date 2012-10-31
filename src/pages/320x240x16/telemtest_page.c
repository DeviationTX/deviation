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
#include "telemetry.h"
#include "gui/gui.h"

#define tp (pagemem.u.telemtest_page)

#define TELEM_FONT NORMALBOX_FONT
#define TELEM_TXT_FONT DEFAULT_FONT
#define TELEM_ERR_FONT NORMALBOXNEG_FONT
static const char *telem_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u32 val = (long)data;
    return TELEMETRY_GetValueStr(tp.str, val);
}

static const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long val = (long)data;
    return TELEMETRY_ShortName(tp.str, val);
}

static void show_page()
{
    const u8 row_height = 20;
    for(long i = 0; i < 4; i++) {
        GUI_CreateLabelBox(20, 40 + i*row_height, 40, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_TEMP1+i));
        tp.temp[i] = GUI_CreateLabelBox(60,  40 + i*row_height, 40, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_TEMP1+1));
    }
    for(long i = 0; i < 3; i++) {
        GUI_CreateLabelBox(120, 40 + i*row_height, 40, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_VOLT1+i));
        tp.volt[i] = GUI_CreateLabelBox(155,  40 + i*row_height, 40, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_VOLT1+i));
    }
    for(long i = 0; i < 2; i++) {
        GUI_CreateLabelBox(220, 40 + i*row_height, 40, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_RPM1+i));
        tp.rpm[i]  = GUI_CreateLabelBox(255,  40 + i*row_height, 40, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_RPM1+i));
    }
    for(long i = 0; i < 5; i++) {
        GUI_CreateLabelBox(20, 140 + i*row_height, 60, 16, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(TELEM_GPS_LAT+i));
        tp.gps[i]  = GUI_CreateLabelBox(100,  140 + i*row_height, 200, 16, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(TELEM_GPS_LAT+i));
    }
    tp.telem = Telemetry;
    tp.telem.time[0] = 0;
    tp.telem.time[1] = 0;
    tp.telem.time[2] = 0;
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if(tp.return_page) {
        PAGE_SetModal(0);
        PAGE_RemoveAllObjects();
        tp.return_page(tp.return_val);
    }
}

void PAGE_TelemtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Telemetry"));
    show_page();
}

void PAGE_TelemtestModal(void(*return_page)(int page), int page)
{
    PAGE_SetModal(1);
    tp.return_page = return_page;
    tp.return_val = page;
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader_ExitOnly(_tr("Telemetry"), okcancel_cb);

    show_page();
}

void PAGE_TelemtestEvent() {
    int i;
    u32 time = CLOCK_getms();
    for(i = 0; i < 3; i++) {
        if (Telemetry.volt[i] != tp.telem.volt[i]) {
            GUI_Redraw(tp.volt[i]);
            tp.telem.volt[i] = Telemetry.volt[i];
        }
    }
    for(i = 0; i < 2; i++) {
        if (Telemetry.rpm[i] != tp.telem.rpm[i]) {
            GUI_Redraw(tp.rpm[i]);
            tp.telem.rpm[i] = Telemetry.rpm[i];
        }
    }
    for(i = 0; i < 4; i++) {
        if (Telemetry.temp[i] != tp.telem.temp[i]) {
            GUI_Redraw(tp.temp[i]);
            tp.telem.temp[i] = Telemetry.temp[i];
        }
    }
    if(memcmp(&tp.telem.gps, &Telemetry.gps, sizeof(struct gps)) != 0) {
        tp.telem.gps = Telemetry.gps;
        for(i = 0; i < 5; i++)
            GUI_Redraw(tp.gps[i]);
    }
    if(Telemetry.time[0] && (time - Telemetry.time[0] > TELEM_ERROR_TIME || tp.telem.time[0] == 0)) {
        struct LabelDesc *font;
        if (time - Telemetry.time[0] > TELEM_ERROR_TIME) {
            font = &TELEM_ERR_FONT;
            Telemetry.time[0] = 0;
        } else {
            font = &TELEM_FONT;
        }
        tp.telem.time[0] = Telemetry.time[0];
        for(i = 0; i < 3; i++)
            GUI_SetLabelDesc(tp.volt[i], font);
        for(i = 0; i < 2; i++)
            GUI_SetLabelDesc(tp.rpm[i], font);
    }
    if(Telemetry.time[1] && (time - Telemetry.time[1] > TELEM_ERROR_TIME || tp.telem.time[1] == 0)) {
        struct LabelDesc *font;
        if (time - Telemetry.time[1] > TELEM_ERROR_TIME) {
            font = &TELEM_ERR_FONT;
            Telemetry.time[1] = 0;
        } else {
            font = &TELEM_FONT;
        }
        tp.telem.time[1] = Telemetry.time[1];
        for(i = 0; i < 4; i++)
            GUI_SetLabelDesc(tp.temp[i], font);
    }
    if(Telemetry.time[2] && (time - Telemetry.time[2] > TELEM_ERROR_TIME || tp.telem.time[2] == 0)) {
        struct LabelDesc *font;
        if (time - Telemetry.time[2] > TELEM_ERROR_TIME) {
            font = &TELEM_ERR_FONT;
            Telemetry.time[2] = 0;
        } else {
            font = &TELEM_FONT;
        }
        tp.telem.time[2] = Telemetry.time[2];
        for(i = 0; i < 5; i++)
            GUI_SetLabelDesc(tp.gps[i], font);
    }
}

