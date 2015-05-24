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
#include "mixer_standard.h"
static struct timer_page * const tp = &pagemem.u.timer_page;
static struct timer_obj * const gui = &gui_objs.u.timer;

const char *timer_str_cb(guiObject_t *obj, const void *data);

static const char *show_timerperm_cb(guiObject_t *obj, const void *data);
static void update_countdown(u8 idx);
static const char *set_source_cb(guiObject_t *obj, int dir, void *data);
static void toggle_source_cb(guiObject_t *obj, void *data);
static void toggle_timertype_cb(guiObject_t *obj, void *data);
static const char *set_timertype_cb(guiObject_t *obj, int dir, void *data);
static const char *set_start_cb(guiObject_t *obj, int dir, void *data);
static void reset_timerperm_cb(guiObject_t *obj, const void *data);
static void _show_page(int page);

void PAGE_TimerInit(int page)
{
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    _show_page(page);
}

void PAGE_TimerEvent()
{
}

void TIMERPAGE_Show(guiObject_t *obj, const void *data)
{
    (void)obj;
    PAGE_TimerInit((long)data);
}

static const char *switch_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    if(Model.timer[i].type == TIMER_STOPWATCH_PROP || Model.timer[i].type == TIMER_COUNTDOWN_PROP) {
        return _tr("Control");
    } else {
        return _tr("Switch");
    }
}

const char *set_timertype_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    u8 changed;
    timer->type = GUI_TextSelectHelper(timer->type, 0, TIMER_LAST - 1, dir, 1, 1, &changed);
    if (changed){
        TIMER_Reset(idx);
        update_countdown(idx);
    }
    switch (timer->type) {
        case TIMER_STOPWATCH: return _tr("stopwatch");
        case TIMER_STOPWATCH_PROP: return _tr("stop-prop");
        case TIMER_COUNTDOWN: return _tr("countdown");
        case TIMER_COUNTDOWN_PROP: return _tr("cntdn-prop");
        case TIMER_PERMANENT: return _tr("permanent");
        case TIMER_LAST: break;
    }
    return "";
}

const char *_set_src_cb(guiTextSelect_t *obj, u8 *src, int dir, int idx)
{
    u8 changed;
    if (Model.mixer_mode == MIXER_STANDARD && Model.type == MODELTYPE_HELI)  { //Improvement: only to intelligent switch setting for heli type in standard mode
        int is_neg = MIXER_SRC_IS_INV(*src);
        int step = mapped_std_channels.throttle + NUM_INPUTS +1;
        int newsrc = GUI_TextSelectHelper(MIXER_SRC(*src), 0, step, dir, step, step, &changed);
        MIXER_SET_SRC_INV(newsrc, is_neg);
        *src = newsrc;
    } else {
        *src = INPUT_SelectSource(*src, dir, &changed, -1);
    }
    if (changed) {
        TIMER_Reset(idx);
    }
    GUI_TextSelectEnablePress(obj, MIXER_SRC(*src));
    return INPUT_SourceName(tempstring, *src);
}

const char *set_resetsrc_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    int idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    return _set_src_cb((guiTextSelect_t *)obj, &timer->resetsrc, dir, idx);
}

void toggle_resetsrc_cb(guiObject_t *obj, void *data)
{
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    if(MIXER_SRC(timer->resetsrc)) {
        MIXER_SET_SRC_INV(timer->resetsrc, ! MIXER_SRC_IS_INV(timer->resetsrc));
        TIMER_Reset(idx);
        GUI_Redraw(obj);
    }
}

const char *timer_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    snprintf(tempstring, sizeof(tempstring), _tr("Timer%d"), i + 1);
    return tempstring;
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    int idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    const char *str = _set_src_cb((guiTextSelect_t *)obj, &timer->src, dir, idx);
    //if (0 && Model.mixer_mode == MIXER_STANDARD)  {
    //    return MIXER_SRC(timer->src) ? _tr("On") : _tr("Off");
    //}
    return str;
}

void toggle_timertype_cb(guiObject_t *obj, void *data)
{
    u8 idx = (long)data;
    struct Timer *timer = &Model.timer[idx];
    timer->type = TIMER_LAST == timer->type + 1 ? 0 : timer->type + 1;
    TIMER_Reset(idx);
    update_countdown(idx);
    GUI_Redraw(obj);
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

const char *show_timerperm_cb(guiObject_t *obj, const void *data)
{
  (void)obj;
  u8 idx = (long)data;
  if (idx & 0x80)
      snprintf(tempstring, sizeof(tempstring), _tr("Set to"));
  else
      TIMER_SetString(tempstring,(long)Model.timer[idx & 0x7f].val);
  return tempstring;
}

