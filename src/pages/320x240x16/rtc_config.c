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
static struct rtc_obj * const gui = &gui_objs.u.rtc;

static const int ADD_OFFSET = 20;   // to add some space in the middle
#define XL(w)  ((LCD_WIDTH / 4 - (w) / 2) - ADD_OFFSET)
#define XM(w)  (LCD_WIDTH / 2 - (w) / 2)
#define XR(w)  ((3 * LCD_WIDTH / 4 - (w) / 2) + ADD_OFFSET)
#define X(r,w) ((r == 0) ? XL(w) : ((r == 1) ? XR(w) : XM(w)))

enum { TIMEBUTTON=DAY+100, DATEBUTTON,  // for buttons setting time and date
       TIMELABEL, DATELABEL,            // for description labels
       ACTTIME, ACTDATE,                // ACT-/NEWTIME and *DATE are used for the formatted values
       SETLABEL, RESULTLABEL,           // for labels at new time/date and result time/date
       NEWTIME, NEWDATE };
u8 order[6] = { DAY, MONTH, YEAR, HOUR, MINUTE, SECOND };

extern const char *timeformats[];
extern const char *dateformats[];

static struct rtc_page * const rp = &pagemem.u.rtc_page;

void _show_page();
static const char *rtc_val_cb(guiObject_t *obj, int dir, void *data);

int min[6], max[6];     // for reordering the values are set dynamically
static const u8 daysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

char newstring[31], resultstring[31];

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
    u32 time = RTC_GetValue();
    u32 timevalue = RTC_GetTimeValue(time);
    Rtc.value[HOUR] = (u16)(timevalue / 3600);
    Rtc.value[MINUTE] = (u16)(timevalue % 3600) / 60;
    Rtc.value[SECOND] = (u16)(timevalue % 60);
    RTC_GetDateStringLong(tempstring,time);
    int idx = (tempstring[1] == '.' ? 1 : 2);
    tempstring[idx] = 0;
    tempstring[idx+3] = 0;
    Rtc.value[DAY] = atoi(tempstring);
    Rtc.value[MONTH] = atoi(tempstring + idx + 1);
    Rtc.value[YEAR] = atoi(tempstring + idx + 4);
    min[SECOND] = 0;             max[SECOND] = 59;
    min[MINUTE] = 0;             max[MINUTE] = 59;
    min[HOUR]   = 0;             max[HOUR]   = 23;
    min[DAY]    = 1;             max[DAY]    = daysInMonth[Rtc.value[MONTH] - 1] + (((Rtc.value[YEAR] % 4) == 0) && (Rtc.value[MONTH] == 2) ? 1 : 0);
    min[MONTH]  = 1;             max[MONTH]  = 12;
    min[YEAR]   = RTC_STARTYEAR; max[YEAR]   = RTC_STARTYEAR + 67;
    _show_page();
}

const char *rtc_show_val_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u32 time = RTC_GetValue();
    switch ((long)data) {
        case SECOND:
            sprintf(tempstring, "%2d", RTC_GetTimeValue(time) % 60);
            break;
        case MINUTE:
            sprintf(tempstring, "%2d", (RTC_GetTimeValue(time) / 60) % 60);
            break;
        case HOUR: {
            u32 value = RTC_GetTimeValue(time) / 3600;
            if (Transmitter.rtcflags & CLOCK12HR) {
                u8 hour = value % 12;
                if (hour == 0)
                    hour = 12;
                sprintf(tempstring, "%2d %s", hour, value > 12? "PM" : "AM");
            } else {
                sprintf(tempstring, "%2d", value % 24);
            }
            break;
        }
        case DAY:
            RTC_GetDateStringLong(tempstring, time);
            tempstring[tempstring[1] == '.' ? 1 : 2] = 0;
            break;
        case MONTH:
            RTC_GetDateStringLong(tempstring, time);
            int idx = (tempstring[1] == '.' ? 1 : 2);
            tempstring[idx+3] = 0;
            if (tempstring[idx + 1] == '0') idx++;
            return tempstring + idx + 1;
        case YEAR:
            RTC_GetDateStringLong(tempstring, time);
            return tempstring + (tempstring[1] == '.' ? 1 : 2) + 4;
    }
    return tempstring;
}

static const char *rtc_text_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    unsigned int idx = (long)data;
    if (idx <= (sizeof(Rtc.value) / sizeof(Rtc.value[0]))) {
        GUI_DrawBackground(X(idx/3, 32) + ((idx%3 == 2) ? 67 : 0) - ((idx%3 == 0) ? 67 : 0), 84, 64, 16);
        idx = order[idx];
    }
    switch (idx) {
        case SECOND:     return _tr("Second");
        case MINUTE:     return _tr("Minute");
        case HOUR:       return _tr("Hour");
        case DAY:        return _tr("Day");
        case MONTH:      return _tr("Month");
        case YEAR:       return _tr("Year");
        case ACTTIME: {
            RTC_GetTimeFormatted(tempstring, RTC_GetValue());
            return tempstring;
        }
        case ACTDATE: {
            RTC_GetDateFormatted(tempstring, RTC_GetValue());
            return tempstring;
        }
        case NEWTIME: {
            RTC_GetTimeFormatted(tempstring, RTC_GetSerial(Rtc.value[YEAR], Rtc.value[MONTH], Rtc.value[DAY], Rtc.value[HOUR], Rtc.value[MINUTE], Rtc.value[SECOND]));
            return tempstring;
        }
        case NEWDATE: {
            RTC_GetDateFormatted(tempstring, RTC_GetSerial(Rtc.value[YEAR], Rtc.value[MONTH], Rtc.value[DAY], Rtc.value[HOUR], Rtc.value[MINUTE], Rtc.value[SECOND]));
            return tempstring;
        }
        case TIMEBUTTON:   return _tr("Set time");
        case DATEBUTTON:   return _tr("Set date");
        case TIMELABEL:    return _tr("Time format");
        case DATELABEL:    return _tr("Date format");
        case SETLABEL:     return _tr("value to set");
        case RESULTLABEL:  return _tr("resulting value");
    }
    return _tr("Unknown");
}

void rtc_set_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if ((long)data == TIMEBUTTON) {
        RTC_SetTime(Rtc.value[HOUR], Rtc.value[MINUTE], Rtc.value[SECOND]);
        GUI_Redraw(&gui->acttime);
    } else if ((long)data == DATEBUTTON) {
        RTC_SetDate(Rtc.value[YEAR], Rtc.value[MONTH], Rtc.value[DAY]);
        GUI_Redraw(&gui->actdate);
    }
}

const char *rtc_select_format_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    if ((long)data == ACTTIME) {
        u8 index = Transmitter.rtcflags & TIMEFMT;
        u8 changed;
        index = GUI_TextSelectHelper(index, 0, RTC_GetNumberTimeFormats() - 1, dir, 1, 1, &changed);
        if (changed) {
            Transmitter.rtcflags = (Transmitter.rtcflags & (0xff ^ TIMEFMT)) | (index & TIMEFMT);
            GUI_Redraw(&gui->acttime);
            GUI_Redraw(&gui->newtime);
            for (u8 i=0; i<sizeof(order); i++)
                if (order[i] == HOUR) {
                    GUI_Redraw(&gui->select[i]);
                    break;
                }
        }
        return timeformats[index];
    }
    if ((long)data == ACTDATE) {
        u8 index = (Transmitter.rtcflags & DATEFMT) >> 4;
        u8 changed;
        index = GUI_TextSelectHelper(index, 0, RTC_GetNumberDateFormats() - 1, dir, 1, 1, &changed);
        if (changed) {
            Transmitter.rtcflags = (Transmitter.rtcflags & (0xff ^ DATEFMT)) | ((index << 4) & DATEFMT);
            GUI_Redraw(&gui->actdate);
            GUI_Redraw(&gui->newdate);
            // reorder spinbuttons to date format
            RTC_GetDateFormattedOrder(index, &order[0], &order[1], &order[2]);
            for (u8 i=0; i<=2; i++) {
                GUI_Redraw(&gui->label[i]);
                GUI_Redraw(&gui->select[i]);
            }
        }
        return dateformats[index];
    }
    return "";
}

void _show_page()
{
    GUI_CreateLabel(&gui->datelbl, XL(128), 40, rtc_text_cb, DEFAULT_FONT, (void *)DATELABEL);
    GUI_CreateLabel(&gui->timelbl, XR(128), 40, rtc_text_cb, DEFAULT_FONT, (void *)TIMELABEL);
    GUI_CreateTextSelect(&gui->dateformat, XL(128), 56, TEXTSELECT_128, NULL, rtc_select_format_cb, (void *)ACTDATE);
    GUI_CreateTextSelect(&gui->timeformat, XR(128), 56, TEXTSELECT_128, NULL, rtc_select_format_cb, (void *)ACTTIME);

    RTC_GetDateFormattedOrder((Transmitter.rtcflags & DATEFMT) >> 4, &order[0], &order[1], &order[2]); // initial ordering

    for (long i=0; i<6; i++) {
        GUI_CreateLabel(&gui->label[i], X(i/3, 32) + ((i%3 == 2) ? 67 : 0) - ((i%3 == 0) ? 67 : 0), 84, rtc_text_cb, DEFAULT_FONT, (void *)i);
        GUI_CreateTextSelect(&gui->select[i], X(i/3, 32) + ((i%3 == 2) ? 67 : 0) - ((i%3 == 0) ? 67 : 0), 100, TEXTSELECT_VERT_64, NULL, rtc_val_cb, (void *)i);
    }

    const int DATEBOXWIDTH = 180;
    u16 w, h;
    LCD_SetFont(DEFAULT_FONT.font);
    LCD_GetStringDimensions((u8 *)rtc_text_cb(NULL, (void *)SETLABEL), &w, &h);
    GUI_CreateLabel(&gui->newlbl, XM(w), 168 - h / 2 , rtc_text_cb, DEFAULT_FONT, (void *)SETLABEL);
    GUI_CreateLabelBox(&gui->newdate, XL(DATEBOXWIDTH), 150, DATEBOXWIDTH, 32, &BIGBOX_FONT, rtc_text_cb, NULL, (void *)NEWDATE);
    GUI_CreateLabelBox(&gui->newtime, XR(DATEBOXWIDTH), 150, DATEBOXWIDTH, 32, &BIGBOX_FONT, rtc_text_cb, NULL, (void *)NEWTIME);

    GUI_CreateButton(&gui->setdate, XL(96), 184, BUTTON_96, rtc_text_cb, rtc_set_cb, (void *)DATEBUTTON);
    GUI_CreateButton(&gui->settime, XR(96), 184, BUTTON_96, rtc_text_cb, rtc_set_cb, (void *)TIMEBUTTON);

    LCD_GetStringDimensions((u8 *)rtc_text_cb(NULL, (void *)RESULTLABEL), &w, &h);
    GUI_CreateLabel(&gui->actlbl, XM(w), 243 - h / 2 , rtc_text_cb, DEFAULT_FONT, (void *)RESULTLABEL);
    GUI_CreateLabelBox(&gui->actdate, XL(DATEBOXWIDTH), 225, DATEBOXWIDTH, 32, &BIGBOX_FONT, rtc_text_cb, NULL, (void *)ACTDATE);
    GUI_CreateLabelBox(&gui->acttime, XR(DATEBOXWIDTH), 225, DATEBOXWIDTH, 32, &BIGBOX_FONT, rtc_text_cb, NULL, (void *)ACTTIME);
}

void PAGE_RTCEvent()
{
    static u32 lastrtcvalue = 0;
    u32 actualrtcvalue = RTC_GetValue();
    if (lastrtcvalue != actualrtcvalue) {
        GUI_Redraw(&gui->acttime);
        GUI_Redraw(&gui->actdate);
        lastrtcvalue = actualrtcvalue;
    }
}

static const char *rtc_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    unsigned int idx = (long)data;
    u8 changed;
    if (idx <= (sizeof(Rtc.value) / sizeof(Rtc.value[0]))) {
        idx = order[idx];
        Rtc.value[idx] = GUI_TextSelectHelper(Rtc.value[idx], min[idx], max[idx], dir, 1, 4, &changed);
        if (changed) {
            if (idx == MONTH || idx == YEAR)
                max[DAY] = daysInMonth[Rtc.value[MONTH] - 1] + (((Rtc.value[YEAR] % 4) == 0) && (Rtc.value[MONTH] == 2) ? 1 : 0);
            GUI_Redraw(&gui->newtime);
            GUI_Redraw(&gui->newdate);
        }
        if (idx == YEAR)
            sprintf(tempstring, "%4d", Rtc.value[idx]);
        else if (idx == HOUR) {
            u8 tmp = Rtc.value[HOUR];
            if (Transmitter.rtcflags & CLOCK12HR) {
                tmp %= 12;
                if (tmp == 0)
                    tmp = 12;
                sprintf(tempstring, "%2d %s", tmp, Rtc.value[HOUR] >= 12? "pm" : "am");
            } else {
                sprintf(tempstring, "%2d", tmp);
            }
        }
        else if (idx == MONTH)
            RTC_GetMonthFormatted(tempstring, Rtc.value[MONTH]);
        else
            sprintf(tempstring, "%2d", Rtc.value[idx]);
    }
    return tempstring;
}

#endif
