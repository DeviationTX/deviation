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
#include "config/model.h"
#include "rtc.h"

#include "../common/_set_timer_page.c"

guiObject_t *firstObj;

const char * _timer_new_str_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    long index = (long)data;
    u8 changed = 0;
    tempstring[0] = '\0';
    if (index == TIMER_SECONDS) {
        permanent.second = GUI_TextSelectHelper(permanent.second, 0, 59, dir, 1, 10, &changed);
        sprintf(tempstring, "%2d", permanent.second);
    } else if (index == TIMER_MINUTES) {
        permanent.minute = GUI_TextSelectHelper(permanent.minute, 0, 59, dir, 1, 10, &changed);
        sprintf(tempstring, "%2d", permanent.minute);
    } else if (index == TIMER_HOURS) {
        permanent.hour   = GUI_TextSelectHelper(permanent.hour,   0, 99, dir, 1, 10, &changed);
        sprintf(tempstring, "%2d", permanent.hour);
    }
    if (changed) GUI_Redraw(&guiset->addvalue);
    return tempstring;
}

void add_set_button_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    permanent.newvalue = ((permanent.hour * 60 + permanent.minute) * 60 + permanent.second) * 1000;
    if ((long)data == ADD_BUTTON) permanent.newvalue += permanent.oldvalue;
    if (permanent.newvalue > 100*60*60*1000) permanent.newvalue = 0;
    GUI_Redraw(&guiset->newvalue);
}

static void _show_settimer_page(u8 index)
{
    memset(&permanent, 0, sizeof(permanent));
    permanent.index = index;
    permanent.oldvalue = TIMER_GetValue(permanent.index);
    permanent.newvalue = TIMER_GetValue(permanent.index);   // set to original value since it is written anyway

    firstObj = NULL;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SETTIMER));
    if (firstObj) {
        GUI_RemoveHierObjects(firstObj);
        firstObj = NULL;
    }
    enum {
        XOFF  = ((LCD_WIDTH - 320) / 2),
        YBOX  = 16,
        YOLD  = 40,
        YRES  = (LCD_HEIGHT - 46 - 8),
        YNEW  = ((YRES + 86) / 2 - 36),
        YUNIT = (YNEW + YBOX),
        YDIFF = 20,
        YBTN  = (YUNIT + YDIFF + YDIFF),
        XLEFT = (XOFF + 30),
        XADD  = (XLEFT),
        XSET  = (XADD + 64),
        XUNIT = (XOFF + 170),
        XVAL  = (XUNIT + 70),
    };

    // actual value
    firstObj = GUI_CreateLabel(&guiset->oldlbl, XLEFT, YOLD, NULL, DEFAULT_FONT, (void *)_tr("actual value"));
    GUI_CreateLabelBox(&guiset->oldvalue, XLEFT, YOLD + 16, 128, 30, &BIGBOX_FONT, timer_value_str_cb, NULL, (void *)OLD_TIMER);

    // value to add or set
    GUI_CreateLabel(&guiset->addlbl, XLEFT, YNEW, NULL, DEFAULT_FONT, (void *)_tr("value to add or set"));
    GUI_CreateLabelBox(&guiset->addvalue, XLEFT, YNEW + 16, 128, 30, &BIGBOX_FONT, timer_value_str_cb, NULL, (void *)ADD_TIMER);

    // select boxes
    GUI_CreateLabel(&guiset->hourlbl, XUNIT, YUNIT + 1, NULL, DEFAULT_FONT, (void *)_tr("Hour"));
    GUI_CreateTextSelect(&guiset->hoursel, XVAL, YUNIT, TEXTSELECT_64, NULL, _timer_new_str_cb, (void *)TIMER_HOURS);
    GUI_CreateLabel(&guiset->minutelbl, XUNIT, YUNIT + YDIFF + 1, NULL, DEFAULT_FONT, (void *)_tr("Minute"));
    GUI_CreateTextSelect(&guiset->minutesel, XVAL, YUNIT + YDIFF, TEXTSELECT_64, NULL, _timer_new_str_cb, (void *)TIMER_MINUTES);
    GUI_CreateLabel(&guiset->secondlbl, XUNIT, YUNIT + YDIFF + YDIFF + 1, NULL, DEFAULT_FONT, (void *)_tr("Second"));
    GUI_CreateTextSelect(&guiset->secondsel, XVAL, YUNIT + YDIFF + YDIFF, TEXTSELECT_64, NULL, _timer_new_str_cb, (void *)TIMER_SECONDS);

    // add / set buttons
    GUI_CreateButton(&guiset->addbtn, XADD, YBTN, BUTTON_64x16, timer_value_str_cb, 0, add_set_button_cb, (void *)ADD_BUTTON);
    GUI_CreateButton(&guiset->setbtn, XSET, YBTN, BUTTON_64x16, timer_value_str_cb, 0, add_set_button_cb, (void *)SET_BUTTON);

    // resulting value
    GUI_CreateLabel(&guiset->newlbl, XLEFT, YRES, NULL, DEFAULT_FONT, (void *)_tr("resulting value"));
    GUI_CreateLabelBox(&guiset->newvalue, XLEFT, YRES + 16, 128, 30, &BIGBOX_FONT, timer_value_str_cb, NULL, (void *)NEW_TIMER);
}
