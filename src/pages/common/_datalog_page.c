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

static struct datalog_obj * const gui = &gui_objs.u.datalog;
static struct datalog     * const dlog = &Model.datalog;
u16 next_update;
u32 remaining;

const char *source_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return DATALOG_Source(tempstring, (long)data);
}

static const char *ratesel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    dlog->rate = GUI_TextSelectHelper(dlog->rate, 0, DLOG_RATE_LAST-1, dir, 1, 1, NULL);
    return DATALOG_RateString(dlog->rate);
}

static const char *sourcesel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    dlog->enable = GUI_TextSelectHelper(dlog->enable, 0, NUM_SOURCES, dir, 1, 1, &changed);
    if (changed)
        DATALOG_UpdateState();
    return INPUT_SourceName(tempstring, dlog->enable);
}

static const char *reset_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Reset");
}

static void reset_press_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    (void)data;
    DATALOG_Reset();
    GUI_Redraw(&gui->remaining);
}

void PAGE_DatalogEvent()
{
    if((u16)(CLOCK_getms() / 1000) > next_update) {
        u32 left = DATALOG_Remaining();
        if(remaining != left) {
            remaining = left;
            next_update = (u16)(CLOCK_getms() / 1000) + 5;
            GUI_Redraw(&gui->remaining);
        }
    }
}
