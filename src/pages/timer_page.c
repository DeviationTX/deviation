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

#include "target.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

static struct timer_page * const tp = &pagemem.u.timer_page;

static void update_countdown(u8 idx);
static const char *set_source_cb(guiObject_t *obj, int dir, void *data);
static void toggle_source_cb(guiObject_t *obj, void *data);
static void toggle_timertype_cb(guiObject_t *obj, void *data);
static const char *set_timertype_cb(guiObject_t *obj, int dir, void *data);
static const char *set_start_cb(guiObject_t *obj, int dir, void *data);

void PAGE_TimerInit(int page)
{
    const char * const timerstr[] = {
        "Timer 1:", "Timer 2:"
    };
    (void)page;
    int i;
    PAGE_SetModal(0);
    for (i = 0; i < NUM_TIMERS; i++) {
        u8 x = 48 + i * 96;
        //Row 1
        GUI_CreateLabel(8, x, NULL, DEFAULT_FONT, (void *)timerstr[i]);
        GUI_CreateTextSelect(72, x, TEXTSELECT_96, 0x0000, toggle_timertype_cb, set_timertype_cb, (void *)(long)i);
        //Row 2
        GUI_CreateLabel(8, x+24, NULL, DEFAULT_FONT, "Switch:");
        GUI_CreateTextSelect(72, x+24, TEXTSELECT_96, 0x0000, toggle_source_cb, set_source_cb, (void *)(long)i);
        //Row 3
        tp->startLabelObj[i] = GUI_CreateLabel(8, x+48, NULL, DEFAULT_FONT, "Start:");
        tp->startObj[i] = GUI_CreateTextSelect(72, x+48, TEXTSELECT_96, 0x0000, NULL, set_start_cb, (void *)(long)i);

        update_countdown(i);
    }
}

void PAGE_TimerEvent()
{
}
void update_countdown(u8 idx)
{
    u8 hide = Model.timer[idx].type == TIMER_STOPWATCH;
    GUI_SetHidden(tp->startObj[idx], hide);
    GUI_SetHidden(tp->startLabelObj[idx], hide);
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    u8 is_neg = MIX_SRC_IS_INV(timer->src);
    u8 changed;
    u8 src = GUI_TextSelectHelper(MIX_SRC(timer->src), 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, &changed);
    MIX_SET_SRC_INV(src, is_neg);
    if (changed) {
        timer->src = src;
    }
    return MIXPAGE_SourceName(src);
}

void toggle_source_cb(guiObject_t *obj, void *data)
{
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    MIX_SET_SRC_INV(timer->src, ! MIX_SRC_IS_INV(timer->src));
    GUI_Redraw(obj);
}

const char *set_timertype_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    timer->type = GUI_TextSelectHelper(timer->type, 0, 1, dir, 1, 1, NULL);
    update_countdown(idx);
    switch (timer->type) {
    case TIMER_STOPWATCH: return "stopwatch";
    case TIMER_COUNTDOWN: return "countdown";
    }
    return "";
}

void toggle_timertype_cb(guiObject_t *obj, void *data)
{
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    timer->type = ! timer->type;
    update_countdown(idx);
    GUI_Redraw(obj);
}

const char *set_start_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    timer->timer = GUI_TextSelectHelper(timer->timer, 0, TIMER_MAX_VAL, dir, 5, 30, NULL);
    u8 h = timer->timer / 3600;
    u8 m = (timer->timer - h*3600) / 60;
    u8 s = timer->timer -h*3600 - m*60;
    sprintf(tp->timer, "%02d:%02d:%02d", h, m, s);
    return tp->timer;
}
