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

#define tp (pagemem.u.telemtest_page)
#define gui1 (&gui_objs.u.telemtest1)
#define gui2 (&gui_objs.u.telemtest2)

#define TELEM_FONT NORMALBOX_FONT
#define TELEM_TXT_FONT DEFAULT_FONT
#define TELEM_ERR_FONT NORMALBOXNEG_FONT

static u8 time_count = 0;
static u8 telem_state_check()
{
    if (PAGE_TelemStateCheck(tp.str, sizeof(tp.str))==0) {
        memset(gui1, 0, sizeof(*gui1));
        return 0;
    }
    return 1;
}

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
    char str[25];
    sprintf(tp.str, "%s:", TELEMETRY_ShortName(str, val));
    return tp.str;
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

void PAGE_TelemtestEvent() {
    time_count ++;
    //if (time_count < 10) // 100ms x10 = 2seconds, to slow down the refresh rate to once per 2s
    //    return;
    time_count = 0;
    int i;
    u32 time = CLOCK_getms();
    for(i = 0; i < 3; i++) {
        if (Telemetry.volt[i] != tp.telem.volt[i]) {
            if (OBJ_IS_USED(&gui1->volt[i]))  // in devo10, the item objects are drawn in different pages, so some of them might not exist in a page
                GUI_Redraw(&gui1->volt[i]);
            tp.telem.volt[i] = Telemetry.volt[i];
        }
    }
    for(i = 0; i < 2; i++) {
        if (Telemetry.rpm[i] != tp.telem.rpm[i]) {
            if (OBJ_IS_USED(&gui1->rpm[i]))
                GUI_Redraw(&gui1->rpm[i]);
            tp.telem.rpm[i] = Telemetry.rpm[i];
        }
    }
    for(i = 0; i < 4; i++) {
        if (Telemetry.temp[i] != tp.telem.temp[i]) {
            if (OBJ_IS_USED(&gui1->temp[i]))
                GUI_Redraw(&gui1->temp[i]);
            tp.telem.temp[i] = Telemetry.temp[i];
        }
    }
    if(memcmp(&tp.telem.gps, &Telemetry.gps, sizeof(struct gps)) != 0) {
        tp.telem.gps = Telemetry.gps;
        for(i = 0; i < 5; i++) {
            guiObject_t *obj = GUI_GetScrollableObj(&gui2->scrollable, i/2, i%2);
            if (obj)
                GUI_Redraw(obj);
        }
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
            if (OBJ_IS_USED(&gui1->volt[i]))
                GUI_SetLabelDesc(&gui1->volt[i], font);
        for(i = 0; i < 2; i++)
            if (OBJ_IS_USED(&gui1->rpm[i]))
                GUI_SetLabelDesc(&gui1->rpm[i], font);
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
            if (OBJ_IS_USED(&gui1->temp[i]))
                GUI_SetLabelDesc(&gui1->temp[i], font);
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
        for(i = 0; i < 5; i++) {
            guiObject_t *obj = GUI_GetScrollableObj(&gui2->scrollable, i/2, i%2);
            if(obj)
                GUI_SetLabelDesc((guiLabel_t *)obj, font);
        }
    }
}

