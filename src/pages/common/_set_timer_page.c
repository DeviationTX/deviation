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

static struct settimer_obj * const guiset = &gui_objs.u.settimer;
static struct timer_page   * const tp     = &pagemem.u.timer_page;

enum { OLD_TIMER=0, ADD_TIMER, NEW_TIMER,               // only for readability
       SET_BUTTON=100, ADD_BUTTON, SELECT_HMS, SELECT_VALUE,
       ADDSET_SELECT,
       TIMER_SECONDS=321, TIMER_MINUTES, TIMER_HOURS };


static void _show_settimer_page(u8 index);

void PAGE_SetTimerInit(int index)
{
    _show_settimer_page(index);
}

void PAGE_SetTimerExit()
{
    TIMER_SetValue(tp->index, tp->newvalue);
    Model.timer[tp->index].val = tp->newvalue;
}

const char * timer_value_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    switch ((long)data) {
        case OLD_TIMER:
            TIMER_SetString(tempstring, tp->oldvalue);
            return tempstring;
        case ADD_TIMER:
            TIMER_SetString(tempstring, ((tp->hour * 60 + tp->minute) * 60 + tp->second) * 1000);
            return tempstring;
        case NEW_TIMER:
            TIMER_SetString(tempstring, tp->newvalue);
            return tempstring;
        case ADD_BUTTON:
            return _tr("Add");
        case SET_BUTTON:
            return _tr("Set");
    }
    return "";
}

