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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

enum {
    LABEL_X = 75,
    LABEL_W = LCD_WIDTH - 75,
    TEXTSEL1_X  = 0,
    TEXTSEL1_W  = 55,
    TEXTSEL2_X  = 75,
    TEXTSEL2_W  = 24,
};
#endif //OVERRIDE_PLACEMENT
// don't include this in Devo7e due to memory restrictions
#if HAS_PERMANENT_TIMER
#include "../common/_set_timer_page.c"

const char *settimer_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)dir;
    u8 changed = 0;
    if ((long)data == SELECT_HMS) {
        permanent.hms = GUI_TextSelectHelper(permanent.hms, TIMER_SECONDS, TIMER_HOURS, dir, 1, 1, &changed);
        switch (permanent.hms) {
            case TIMER_SECONDS:
                return _tr("Second");
            case TIMER_MINUTES:
                return _tr("Minute");
            case TIMER_HOURS:
                return _tr("Hour");
        }
        return "";
    }
    if ((long)data == SELECT_VALUE) {
        tempstring[0] = '\0';
        switch (permanent.hms) {
            case TIMER_SECONDS:
                permanent.second = GUI_TextSelectHelper(permanent.second, 0, 59, dir, 1, 10, &changed);
                sprintf(tempstring, "%d", permanent.second);
                break;
            case TIMER_MINUTES:
                permanent.minute = GUI_TextSelectHelper(permanent.minute, 0, 59, dir, 1, 10, &changed);
                sprintf(tempstring, "%d", permanent.minute);
                break;
            case TIMER_HOURS:
                permanent.hour = GUI_TextSelectHelper(permanent.hour, 0, 99, dir, 1, 10, &changed);
                sprintf(tempstring, "%d", permanent.hour);
                break;
        }
        if (changed) GUI_Redraw(&guiset->addtime);
        return tempstring;
    }
    if ((long)data == ADDSET_SELECT) {
        permanent.addset = GUI_TextSelectHelper(permanent.addset, SET_BUTTON, ADD_BUTTON, dir, 1, 1, NULL);
        return permanent.addset == SET_BUTTON ? _tr("Set") : _tr("Add");
    }
    return "";
}

void add_set_button_cb(struct guiObject *obj, void *data)
{
    (void)obj;
    permanent.newvalue = ((permanent.hour * 60 + permanent.minute) * 60 + permanent.second) * 1000;
    if ((long)data == ADDSET_SELECT && permanent.addset == ADD_BUTTON)
        permanent.newvalue += permanent.oldvalue;
    if (permanent.newvalue > 100*60*60*1000) permanent.newvalue = 0;
    GUI_Redraw(&guiset->newvalue);
}

static void _show_settimer_page(u8 index)
{
    memset(&permanent, 0, sizeof(permanent));
    permanent.index = index;
    permanent.oldvalue = TIMER_GetValue(permanent.index);
    permanent.newvalue = TIMER_GetValue(permanent.index);   // set to original value since it is written anyway
    permanent.addset = ADD_BUTTON;
    permanent.hms = TIMER_HOURS;

    snprintf(tempstring, sizeof(tempstring), "%s %d", _tr(PAGE_GetName(PAGEID_SETTIMER)), index + 1);
    PAGE_ShowHeader(tempstring);

    u8 space = LINE_HEIGHT;
    u8 y = LINE_HEIGHT; // under headline
    //Row 1
    GUI_CreateLabelBox(&guiset->oldtime, LABEL_X, y, LABEL_W, LINE_HEIGHT, &DEFAULT_FONT, timer_value_str_cb, NULL, (void *)(long)OLD_TIMER);

    //Row 2
    y += space;
    GUI_CreateTextSelectPlate(&guiset->hms, TEXTSEL1_X, y, TEXTSEL1_W, LINE_HEIGHT, &DEFAULT_FONT, NULL, settimer_select_cb, (void *)(long)SELECT_HMS);
    GUI_CreateTextSelectPlate(&guiset->value, TEXTSEL2_X, y, TEXTSEL2_W, LINE_HEIGHT, &DEFAULT_FONT, NULL, settimer_select_cb, (void *)(long)SELECT_VALUE);

    //Row 3
    y += space;
    GUI_CreateLabelBox(&guiset->addtime, LABEL_X, y, LABEL_W, LINE_HEIGHT, &DEFAULT_FONT, timer_value_str_cb, NULL, (void *)(long)ADD_TIMER);
    GUI_CreateTextSelectPlate(&guiset->addset, TEXTSEL1_X, y , TEXTSEL1_W, LINE_HEIGHT, &DEFAULT_FONT, add_set_button_cb, settimer_select_cb, (void *)(long)ADDSET_SELECT);
    y += space;

    //Row 4
    GUI_CreateLabelBox(&guiset->newvalue, LABEL_X, y, LABEL_W, LINE_HEIGHT, &DEFAULT_FONT, timer_value_str_cb, NULL, (void *)(long)NEW_TIMER);

}

#endif // HAS_PERMANENT_TIMER
