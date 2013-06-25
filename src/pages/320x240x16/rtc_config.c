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
#include "gui/gui.h"
#include "rtc.h"

#include "target_defs.h"

#if HAS_RTC
#define gui (&gui_objs.u.rtc)

static struct rtc_page * const rp = &pagemem.u.rtc_page;

void _show_page();
static const char *rtc_val_cb(guiObject_t *obj, int dir, void *data);

const int min[6] = { 0, 0, 0, 1, 1, RTC_STARTYEAR };
int max[6] = { 59, 59, 23, 31, 12, RTC_STARTYEAR+67 };    // not const because max[3] will change (day)
static const u8 daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

struct Rtc {
    u16 value[6];
    int clocksource;
    int prescaler;
    int clocksourceready;
} Rtc;

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    RTC_SetValue(RTC_GetSerial(Rtc.value[3], Rtc.value[4], Rtc.value[5]-RTC_STARTYEAR, Rtc.value[2], Rtc.value[1], Rtc.value[0]));
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ChangeByID(PAGEID_TXCFG);
}

void PAGE_RTCInit(int page)
{
    (void)page;
    PAGE_SetModal(1);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_RTC), okcancel_cb);
    u32 timevalue = RTC_GetTimeValue();
    Rtc.value[2] = (u16)(timevalue / 3600);
    Rtc.value[1] = (u16)(timevalue % 3600) / 60;
    Rtc.value[0] = (u16)(timevalue % 60);
    RTC_GetDateStringLong(rp->tmpstr,RTC_GetValue());
    int idx = (rp->tmpstr[1] == '.' ? 1 : 2);
    rp->tmpstr[idx] = 0;
    rp->tmpstr[idx+3] = 0;
    Rtc.value[3] = atoi(rp->tmpstr);
    Rtc.value[4] = atoi(rp->tmpstr + idx + 1);
    Rtc.value[5] = atoi(rp->tmpstr + idx + 4);
    _show_page();
}

const char *rtc_show_val_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    switch ((long)data) {
        case 0:    // second
            sprintf(rp->tmpstr, "%02d", RTC_GetTimeValue() % 60);
            break;
        case 1:    // minute
            sprintf(rp->tmpstr, "%02d", (RTC_GetTimeValue() / 60) % 60);
            break;
        case 2:    // hour
            sprintf(rp->tmpstr, "%02d", (RTC_GetTimeValue() / 3600) % 24);
            break;
        case 3: // day
            RTC_GetDateStringLong(rp->tmpstr,RTC_GetValue());
            rp->tmpstr[rp->tmpstr[1] == '.' ? 1 : 2] = 0;
            break;
        case 4: // month
            RTC_GetDateStringLong(rp->tmpstr,RTC_GetValue());
            int idx = (rp->tmpstr[1] == '.' ? 1 : 2);
            rp->tmpstr[idx+3] = 0;
            return rp->tmpstr + idx + 1;
        case 5: // year
            RTC_GetDateStringLong(rp->tmpstr,RTC_GetValue());
            return rp->tmpstr + (rp->tmpstr[1] == '.' ? 1 : 2) + 4;
    }
    return rp->tmpstr;
}

void _show_page()
{
    int row = 40;
    GUI_CreateLabel(&gui->secondlbl, 72, row, NULL, DEFAULT_FONT, _tr("Second:"));
    GUI_CreateTextSelect(&gui->second, 136, row, TEXTSELECT_64, NULL, rtc_val_cb, (void *)0);
    GUI_CreateLabelBox(&gui->secondvalue, 226, row-1, 24, 18, &SMALLBOX_FONT, rtc_show_val_cb, NULL, (void *)0);
    row += 20;
    GUI_CreateLabel(&gui->minutelbl, 72, row, NULL, DEFAULT_FONT, _tr("Minute:"));
    GUI_CreateTextSelect(&gui->minute, 136, row, TEXTSELECT_64, NULL, rtc_val_cb, (void *)1);
    GUI_CreateLabelBox(&gui->minutevalue, 226, row-1, 24, 18, &SMALLBOX_FONT, rtc_show_val_cb, NULL, (void *)1);
    row += 20;
    GUI_CreateLabel(&gui->hourlbl, 72, row, NULL, DEFAULT_FONT, _tr("Hour:"));
    GUI_CreateTextSelect(&gui->hour, 136, row, TEXTSELECT_64, NULL, rtc_val_cb, (void *)2);
    GUI_CreateLabelBox(&gui->hourvalue, 226, row-1, 24, 18, &SMALLBOX_FONT, rtc_show_val_cb, NULL, (void *)2);
    row += 20;
    GUI_CreateLabel(&gui->daylbl, 72, row, NULL, DEFAULT_FONT, _tr("Day:"));
    GUI_CreateTextSelect(&gui->day, 136, row, TEXTSELECT_64, NULL, rtc_val_cb, (void *)3);
    GUI_CreateLabelBox(&gui->dayvalue, 226, row-1, 24, 18, &SMALLBOX_FONT, rtc_show_val_cb, NULL, (void *)3);
    row += 20;
    GUI_CreateLabel(&gui->monthlbl, 72, row, NULL, DEFAULT_FONT, _tr("Month:"));
    GUI_CreateTextSelect(&gui->month, 136, row, TEXTSELECT_64, NULL, rtc_val_cb, (void *)4);
    GUI_CreateLabelBox(&gui->monthvalue, 226, row-1, 24, 18, &SMALLBOX_FONT, rtc_show_val_cb, NULL, (void *)4);
    row += 20;
    GUI_CreateLabel(&gui->yearlbl, 72, row, NULL, DEFAULT_FONT, _tr("Year:"));
    GUI_CreateTextSelect(&gui->year, 136, row, TEXTSELECT_64, NULL, rtc_val_cb, (void *)5);
    GUI_CreateLabelBox(&gui->yearvalue, 218, row-1, 40, 18, &SMALLBOX_FONT, rtc_show_val_cb, NULL, (void *)5);
}

void PAGE_RTCEvent()
{
    static u32 lastrtcvalue = 0;
    u32 actualrtcvalue = RTC_GetValue();
    if (lastrtcvalue != actualrtcvalue) {
        GUI_Redraw(&gui->secondvalue);
        GUI_Redraw(&gui->minutevalue);
        GUI_Redraw(&gui->hourvalue);
        lastrtcvalue = actualrtcvalue;
    }
}

static const char *rtc_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    if (idx < 6) {
        if (idx == 3) max[3] = daysInMonth[Rtc.value[4] - 1] + (Rtc.value[5] % 4 == 0 ? 1 : 0);
        Rtc.value[idx] = GUI_TextSelectHelper(Rtc.value[idx], min[idx], max[idx], dir, 1, 4, NULL);
        if (idx == 5) sprintf(rp->tmpstr, "%4d", Rtc.value[idx]);
        else sprintf(rp->tmpstr, "%2d", Rtc.value[idx]);
    }
    return rp->tmpstr;
}
#endif
