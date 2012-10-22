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

#define TELEM_FONT TITLE_FONT
static const char *telem_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u32 val = (long)data;
    return TELEMETRY_SetString(tp.str, val);
}

static const char *raw_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u32 val = (long)data;
    return TELEMETRY_GetGPS(tp.str, val);
}

static void show_page()
{
    u16 y = 40;
    GUI_CreateLabelBox(20,  y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("Volt1:"));
    tp.volt[0] = GUI_CreateLabelBox(55,  y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)0L);
    GUI_CreateLabelBox(130, y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("Volt2:"));
    tp.volt[1] = GUI_CreateLabelBox(165, y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)1L);
    GUI_CreateLabelBox(240, y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("Volt3:"));
    tp.volt[2] = GUI_CreateLabelBox(275, y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)2L);
    y = 60;
    GUI_CreateLabelBox(10, y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("Temp1:"));
    tp.temp[0] = GUI_CreateLabelBox(50,  y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)3L);
    GUI_CreateLabelBox(90, y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("Temp2:"));
    tp.temp[1] = GUI_CreateLabelBox(130, y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)4L);
    GUI_CreateLabelBox(170, y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("Temp3:"));
    tp.temp[2] = GUI_CreateLabelBox(210, y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)5L);
    GUI_CreateLabelBox(250, y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("Temp4:"));
    tp.temp[3] = GUI_CreateLabelBox(290, y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)6L);
    y = 80;
    GUI_CreateLabelBox(90,  y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("RPM1:"));
    tp.rpm[0] = GUI_CreateLabelBox(125, y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)7L);
    GUI_CreateLabelBox(170, y, 40, 16, &TELEM_FONT, NULL, NULL, _tr("RPM2:"));
    tp.rpm[1] = GUI_CreateLabelBox(205, y, 40, 16, &ERROR_FONT, telem_cb, NULL, (void *)8L);

    tp.line[0] = GUI_CreateLabelBox(20,  140, 280, 16, &ERROR_FONT, raw_cb, NULL, (void *)0L);
    tp.line[1] = GUI_CreateLabelBox(20,  160, 280, 16, &ERROR_FONT, raw_cb, NULL, (void *)1L);
    tp.line[2] = GUI_CreateLabelBox(20,  180, 280, 16, &ERROR_FONT, raw_cb, NULL, (void *)2L);
    tp.line[3] = GUI_CreateLabelBox(20,  200, 280, 16, &ERROR_FONT, raw_cb, NULL, (void *)3L);
    tp.line[4] = GUI_CreateLabelBox(20,  220, 280, 16, &ERROR_FONT, raw_cb, NULL, (void *)4L);
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
            GUI_Redraw(tp.line[i]);
    }
    if(Telemetry.time[0] && (time - Telemetry.time[0] > TELEM_ERROR_TIME || tp.telem.time[0] == 0)) {
        struct LabelDesc *font;
        if (time - Telemetry.time[0] > TELEM_ERROR_TIME) {
            font = &ERROR_FONT;
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
            font = &ERROR_FONT;
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
            font = &ERROR_FONT;
            Telemetry.time[2] = 0;
        } else {
            font = &TELEM_FONT;
        }
        tp.telem.time[2] = Telemetry.time[2];
        for(i = 0; i < 5; i++)
            GUI_SetLabelDesc(tp.line[i], font);
    }
}

