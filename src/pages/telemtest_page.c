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
    int i;
    u8 *ptr = (val == 1) ? Telemetry.line1 : Telemetry.line2;
    for(i = 0; i < 12; i++) {
        sprintf(tp.str + 2*i, "%02x", *ptr++);
    }
    return tp.str;
}

void PAGE_TelemtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Telemetry"));

    u16 y = 40;
    GUI_CreateLabelBox(20,  y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Volt1:"));
    tp.volt[0] = GUI_CreateLabelBox(55,  y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)0L);
    GUI_CreateLabelBox(130, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Volt2:"));
    tp.volt[1] = GUI_CreateLabelBox(165, y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)1L);
    GUI_CreateLabelBox(240, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Volt3:"));
    tp.volt[2] = GUI_CreateLabelBox(275, y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)2L);
    y = 60;
    GUI_CreateLabelBox(10, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp1:"));
    tp.temp[0] = GUI_CreateLabelBox(50,  y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)3L);
    GUI_CreateLabelBox(90, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp2:"));
    tp.temp[1] = GUI_CreateLabelBox(130, y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)4L);
    GUI_CreateLabelBox(170, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp3:"));
    tp.temp[2] = GUI_CreateLabelBox(210, y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)5L);
    GUI_CreateLabelBox(250, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp4:"));
    tp.temp[3] = GUI_CreateLabelBox(290, y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)6L);
    y = 80;
    GUI_CreateLabelBox(20,  y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("RPM1:"));
    tp.rpm[0] = GUI_CreateLabelBox(55,  y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)7L);
    GUI_CreateLabelBox(130, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("RPM2:"));
    tp.rpm[1] = GUI_CreateLabelBox(165, y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)8L);
    GUI_CreateLabelBox(240, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("RPM3:"));
    tp.rpm[2] = GUI_CreateLabelBox(275, y, 40, 16, &DEFAULT_FONT, telem_cb, NULL, (void *)9L);

    tp.line[0] = GUI_CreateLabelBox(20,  180, 280, 16, &DEFAULT_FONT, raw_cb, NULL, (void *)1L);
    tp.line[1] = GUI_CreateLabelBox(20,  200, 280, 16, &DEFAULT_FONT, raw_cb, NULL, (void *)2L);
    tp.telem = Telemetry;
}
void PAGE_TelemtestEvent() {
    int i;
    for(i = 0; i < 3; i++) {
        if (Telemetry.volt[i] != tp.telem.volt[i]) {
            GUI_Redraw(tp.volt[i]);
            tp.telem.volt[i] = Telemetry.volt[i];
        }
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
    if (memcmp(tp.telem.line1, Telemetry.line1, 12) != 0) {
        GUI_Redraw(tp.line[0]);
        memcpy(tp.telem.line1, Telemetry.line1, 12);
    }
    if (memcmp(tp.telem.line2, Telemetry.line2, 12) != 0) {
        GUI_Redraw(tp.line[1]);
        memcpy(tp.telem.line2, Telemetry.line2, 12);
    }
}
