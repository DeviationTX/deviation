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

#include "../common/_usb_page.c"

#define gui (&gui_objs.u.usb)

const char *show_usb_time_cb(guiObject_t *obj, const void *data);
const char *show_usb_date_cb(guiObject_t *obj, const void *data);

static void _draw_page(u8 enable)
{
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_USB));
#if HAS_RTC
    GUI_CreateLabelBox(&gui->time, (LCD_WIDTH - (113 + 113 +20)) / 2, 40, 113, 28, &SMALLBOX_FONT, show_usb_time_cb, NULL, NULL);
    GUI_CreateLabelBox(&gui->date, (LCD_WIDTH - (113 + 113 +20)) / 2 + 96 + 20, 40, 113, 28, &SMALLBOX_FONT, show_usb_date_cb, NULL, NULL);
#endif

    GUI_CreateLabelBox(&gui->headline, LCD_WIDTH/2-100, 60, 200, 40, &MODELNAME_FONT, NULL, NULL, "www.deviationtx.com");
    sprintf(tempstring, "%s\n%s\n\n%s... %s\n%s %s",
            _tr("Deviation FW version:"), DeviationVersion,
            _tr("USB Filesystem is currently "), enable == 0 ? _tr("Off") : _tr("On"),
            _tr("Press 'Ent' to turn USB Filesystem"),
            enable == 0 ? _tr("On") : _tr("Off"));
    GUI_CreateLabelBox(&gui->msg, LCD_WIDTH/2-126, 120, 252, LCD_HEIGHT-120, &DEFAULT_FONT, NULL, NULL, tempstring);
}

#if HAS_RTC
const char *show_usb_time_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    RTC_GetTimeFormatted(up->datetime, RTC_GetValue());
    return up->datetime;
}

const char *show_usb_date_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    RTC_GetDateFormatted(up->datetime, RTC_GetValue());
    return up->datetime;
}

void PAGE_USBEvent()
{
    u32 time = RTC_GetValue();
    if(time != up->timeval) {
        GUI_Redraw(&gui->time);
    }
    if(RTC_GetDateValue(time)  != RTC_GetDateValue(up->timeval)) {
        GUI_Redraw(&gui->date);
    }
    up->timeval = time;
}
#else //HAS_RTC
void PAGE_USBEvent() {}
#endif
