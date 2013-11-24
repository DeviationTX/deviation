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
    u8 changed;
    tp->tmpstr[0] = '\0';
    if (index == TIMER_SECONDS) {
        permanent.second = GUI_TextSelectHelper(permanent.second, 0, 59, dir, 1, 10, &changed);
        sprintf(tp->tmpstr, "%2d", permanent.second);
    } else if (index == TIMER_MINUTES) {
        permanent.minute = GUI_TextSelectHelper(permanent.minute, 0, 59, dir, 1, 10, &changed);
        sprintf(tp->tmpstr, "%2d", permanent.minute);
    } else if (index == TIMER_HOURS) {
        permanent.hour   = GUI_TextSelectHelper(permanent.hour,   0, 99, dir, 1, 10, &changed);
        sprintf(tp->tmpstr, "%2d", permanent.hour);
    }
    if (changed) GUI_Redraw(&guiset->addvalue);
    return tp->tmpstr;
}

void add_set_button_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    permanent.newvalue = ((permanent.hour * 60 + permanent.minute) * 60 + permanent.second) * 1000;
    if ((long)data == ADD_BUTTON) permanent.newvalue += permanent.oldvalue;
    if (permanent.newvalue > 100*60*60*1000) permanent.newvalue = 0;
    GUI_Redraw(&guiset->newvalue);
}

void exit_page(guiObject_t *obj, const void *data)
{
    (void)data;
    TIMER_SetValue(permanent.index, permanent.newvalue);
    Model.timer[permanent.index].val = permanent.newvalue;
    TIMERPAGE_Show(obj, (void *)(long)(LCD_WIDTH == 480 ? 0 : (permanent.index / 2)));   // 2 timers per page on 320x240-screen - show page 2 for timer 3 and 4
}

static void _show_settimer_page(u8 index)
{
    memset(&permanent, 0, sizeof(permanent));
    permanent.index = index;
    permanent.oldvalue = TIMER_GetValue(permanent.index);
    permanent.newvalue = TIMER_GetValue(permanent.index);   // set to original value since it is written anyway

    firstObj = NULL;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_SETTIMER), exit_page);
    if (firstObj) {
        GUI_RemoveHierObjects(firstObj);
        firstObj = NULL;
    }

    int row = 40, col = 30;

    firstObj = GUI_CreateLabel(&guiset->oldlbl, col, row, NULL, DEFAULT_FONT, (void *)_tr("actual value"));
    GUI_CreateLabelBox(&guiset->oldvalue, col, row + 16, 128, 30, &BIGBOX_FONT, timer_value_str_cb, NULL, (void *)OLD_TIMER);
    row += 50;
    GUI_CreateLabel(&guiset->addlbl, col, row, NULL, DEFAULT_FONT, (void *)_tr("value to add or set"));
    GUI_CreateLabelBox(&guiset->addvalue, col, row + 16, 128, 30, &BIGBOX_FONT, timer_value_str_cb, NULL, (void *)ADD_TIMER);
    // select boxes
    GUI_CreateLabel(&guiset->hourlbl, col + 140, row + 1, NULL, DEFAULT_FONT, (void *)_tr("Hour"));
    GUI_CreateTextSelect(&guiset->hoursel, col + 180, row, TEXTSELECT_64, NULL, _timer_new_str_cb, (void *)TIMER_HOURS);
    GUI_CreateLabel(&guiset->minutelbl, col + 140, row + 21, NULL, DEFAULT_FONT, (void *)_tr("Minute"));
    GUI_CreateTextSelect(&guiset->minutesel, col + 180, row + 20, TEXTSELECT_64, NULL, _timer_new_str_cb, (void *)TIMER_MINUTES);
    GUI_CreateLabel(&guiset->secondlbl, col + 140, row + 41, NULL, DEFAULT_FONT, (void *)_tr("Second"));
    GUI_CreateTextSelect(&guiset->secondsel, col + 180, row + 40, TEXTSELECT_64, NULL, _timer_new_str_cb, (void *)TIMER_SECONDS);
    row += 50;
    GUI_CreateButton(&guiset->addbtn, col, row, BUTTON_64x16, timer_value_str_cb, 0, add_set_button_cb, (void *)ADD_BUTTON);
    GUI_CreateButton(&guiset->setbtn, col + 64, row, BUTTON_64x16, timer_value_str_cb, 0, add_set_button_cb, (void *)SET_BUTTON);
    row += 20;
    GUI_CreateLabel(&guiset->newlbl, col, row, NULL, DEFAULT_FONT, (void *)_tr("resulting value"));
    GUI_CreateLabelBox(&guiset->newvalue, col, row + 16, 128, 30, &BIGBOX_FONT, timer_value_str_cb, NULL, (void *)NEW_TIMER);
    row += 50;
}
