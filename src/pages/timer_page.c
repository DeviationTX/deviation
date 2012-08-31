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

static struct timer_page * const tp = &pagemem.u.timer_page;

static void update_countdown(u8 idx);
const char *timer_str_cb(guiObject_t *obj, const void *data);
static const char *set_source_cb(guiObject_t *obj, int dir, void *data);
static void toggle_source_cb(guiObject_t *obj, void *data);
static void toggle_timertype_cb(guiObject_t *obj, void *data);
static const char *set_timertype_cb(guiObject_t *obj, int dir, void *data);
static const char *set_start_cb(guiObject_t *obj, int dir, void *data);

void PAGE_TimerInit(int page)
{
    (void)page;
    int i;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Timer"));

    for (i = 0; i < NUM_TIMERS; i++) {
        u8 x = 48 + i * 96;
        //Row 1
        GUI_CreateLabel(8, x, timer_str_cb, DEFAULT_FONT, (void *)(long)i);
        GUI_CreateTextSelect(72, x, TEXTSELECT_96, 0x0000, toggle_timertype_cb, set_timertype_cb, (void *)(long)i);
        //Row 2
        GUI_CreateLabel(8, x+24, NULL, DEFAULT_FONT, _tr("Switch:"));
        GUI_CreateTextSelect(72, x+24, TEXTSELECT_96, 0x0000, toggle_source_cb, set_source_cb, (void *)(long)i);
        //Row 3
        tp->startLabelObj[i] = GUI_CreateLabel(8, x+48, NULL, DEFAULT_FONT, _tr("Start:"));
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

const char *timer_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    sprintf(tp->tmpstr, "%s %d:", _tr("Timer"), i + 1);
    return tp->tmpstr;
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    u8 is_neg = MIXER_SRC_IS_INV(timer->src);
    u8 changed;
    u8 src = GUI_TextSelectHelper(MIXER_SRC(timer->src), 0, NUM_SOURCES, dir, 1, 1, &changed);
    MIXER_SET_SRC_INV(src, is_neg);
    if (changed) {
        timer->src = src;
        TIMER_Reset(idx);
    }
    GUI_TextSelectEnablePress(obj, MIXER_SRC(src));
    return INPUT_SourceName(tp->tmpstr, src);
}

void toggle_source_cb(guiObject_t *obj, void *data)
{
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    if(MIXER_SRC(timer->src)) {
        MIXER_SET_SRC_INV(timer->src, ! MIXER_SRC_IS_INV(timer->src));
        TIMER_Reset(idx);
        GUI_Redraw(obj);
    }
}

const char *set_timertype_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    u8 changed;
    struct Timer *timer = &Model.timer[idx];
    timer->type = GUI_TextSelectHelper(timer->type, 0, 1, dir, 1, 1, &changed);
    if (changed)
        TIMER_Reset(idx);
    update_countdown(idx);
    switch (timer->type) {
    case TIMER_STOPWATCH: return _tr("stopwatch");
    case TIMER_COUNTDOWN: return _tr("countdown");
    }
    return "";
}

void toggle_timertype_cb(guiObject_t *obj, void *data)
{
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    timer->type = ! timer->type;
    TIMER_Reset(idx);
    update_countdown(idx);
    GUI_Redraw(obj);
}

const char *set_start_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 idx = (long)data;
    u8 changed;
    struct Timer *timer = &Model.timer[idx];
    timer->timer = GUI_TextSelectHelper(timer->timer, 0, TIMER_MAX_VAL, dir, 5, 30, &changed);
    if (changed)
        TIMER_Reset(idx);
    TIMER_SetString(tp->timer, timer->timer*1000);
    return tp->timer;
}
