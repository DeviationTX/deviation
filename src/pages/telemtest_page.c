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

static const char *volt_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u32 val = Telemetry.volt[(long)data];
    sprintf(tp.str, "%d.%dV", val/10, val % 10);
    return tp.str;
}

static const char *temp_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u32 val = Telemetry.temp[(long)data];
    sprintf(tp.str, "%dC", val);
    return tp.str;
}

static const char *rpm_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u32 val = Telemetry.rpm[(long)data];
    sprintf(tp.str, "%d", val);
    return tp.str;
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
    tp.volt[0] = GUI_CreateLabelBox(55,  y, 40, 16, &DEFAULT_FONT, volt_cb, NULL, (void *)0L);
    GUI_CreateLabelBox(130, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Volt2:"));
    tp.volt[1] = GUI_CreateLabelBox(165, y, 40, 16, &DEFAULT_FONT, volt_cb, NULL, (void *)1L);
    GUI_CreateLabelBox(240, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Volt3:"));
    tp.volt[2] = GUI_CreateLabelBox(275, y, 40, 16, &DEFAULT_FONT, volt_cb, NULL, (void *)2L);
    y = 60;
    GUI_CreateLabelBox(10, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp1:"));
    tp.temp[0] = GUI_CreateLabelBox(50,  y, 40, 16, &DEFAULT_FONT, temp_cb, NULL, (void *)0L);
    GUI_CreateLabelBox(90, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp2:"));
    tp.temp[1] = GUI_CreateLabelBox(130, y, 40, 16, &DEFAULT_FONT, temp_cb, NULL, (void *)1L);
    GUI_CreateLabelBox(170, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp3:"));
    tp.temp[2] = GUI_CreateLabelBox(210, y, 40, 16, &DEFAULT_FONT, temp_cb, NULL, (void *)2L);
    GUI_CreateLabelBox(250, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("Temp4:"));
    tp.temp[3] = GUI_CreateLabelBox(290, y, 40, 16, &DEFAULT_FONT, temp_cb, NULL, (void *)3L);
    y = 80;
    GUI_CreateLabelBox(20,  y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("RPM1:"));
    tp.rpm[0] = GUI_CreateLabelBox(55,  y, 40, 16, &DEFAULT_FONT, rpm_cb, NULL, (void *)0L);
    GUI_CreateLabelBox(130, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("RPM2:"));
    tp.rpm[1] = GUI_CreateLabelBox(165, y, 40, 16, &DEFAULT_FONT, rpm_cb, NULL, (void *)1L);
    GUI_CreateLabelBox(240, y, 40, 16, &DEFAULT_FONT, NULL, NULL, _tr("RPM3:"));
    tp.rpm[2] = GUI_CreateLabelBox(275, y, 40, 16, &DEFAULT_FONT, rpm_cb, NULL, (void *)2L);

    tp.line[0] = GUI_CreateLabelBox(20,  180, 280, 16, &DEFAULT_FONT, raw_cb, NULL, (void *)1L);
    tp.line[1] = GUI_CreateLabelBox(20,  200, 280, 16, &DEFAULT_FONT, raw_cb, NULL, (void *)2L);
    
}
void PAGE_TelemtestEvent() {
    int i;
    for(i = 0; i < 3; i++) {
        GUI_Redraw(tp.volt[i]);
        GUI_Redraw(tp.rpm[i]);
    }
    for(i = 0; i < 4; i++) {
        GUI_Redraw(tp.temp[i]);
    }
    GUI_Redraw(tp.line[0]);
    GUI_Redraw(tp.line[1]);
}
