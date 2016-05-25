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

#include "../common/_timer_page.c"

enum {
    TIMERCOLUMNS   = (LCD_WIDTH == 480 ? 2 : 1),
};


void press_set_cb(guiObject_t *obj, const void *data);
const char *show_set_cb(guiObject_t *obj, const void *data);
const char *show_time_cb(guiObject_t *obj, const void *data);
const char *show_date_cb(guiObject_t *obj, const void *data);

/*            Advanced                     Standard
   Row1       Timer                        Timer
   Row2       Switch                       Switch
   Row3       Reset(perm)/ResetSwitch      Reset(perm)/Start
   Row4       Start/Set-to                 Set-to
*/
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    int COL1 = 30;
    int COL2 = 130;
    int selectable = 0;
    for (int j = 0; j < TIMERCOLUMNS; j++) {
        int row = y;
        int i = relrow * TIMERCOLUMNS + j;
        int timer_num = TIMERCOLUMNS*absrow+j;
        //Row 1
        GUI_CreateLabelBox(&gui->timer[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT,timer_str_cb, NULL, (void *)(long)timer_num);

        GUI_CreateTextSelect(&gui->type[i], COL2, row, TEXTSELECT_96, NULL, set_timertype_cb, (void *)(long)timer_num);
        //Row 2
        row+=20;
        GUI_CreateLabelBox(&gui->switchlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, switch_str_cb, NULL, (void *)(long)timer_num);
        GUI_CreateTextSource(&gui->src[i],  COL2, row, TEXTSELECT_96, toggle_source_cb, set_source_cb, set_input_source_cb, (void *)(long)timer_num);
        //Row 3
        row+=20;
        /* Reset Perm timer*/
        GUI_CreateLabelBox(&gui->resetpermlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Reset"));
        GUI_CreateButton(&gui->resetperm[i], COL2, row, BUTTON_96x16, show_timerperm_cb, 0x0000, reset_timerperm_cb, (void *)(long)timer_num);
        if(Model.mixer_mode != MIXER_STANDARD) {
            /* or Reset switch */
            GUI_CreateLabelBox(&gui->resetlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Reset sw"));
            GUI_CreateTextSource(&gui->resetsrc[i],  COL2, row, TEXTSELECT_96, toggle_resetsrc_cb, set_resetsrc_cb, set_input_rstsrc_cb, (void *)(long)timer_num);
            row+=20;
        }
        //Row 4
        GUI_CreateLabelBox(&gui->startlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Start"));
        GUI_CreateTextSelect(&gui->start[i], COL2, row, TEXTSELECT_96, NULL, set_start_cb, (void *)(long)timer_num);
        if(Model.mixer_mode == MIXER_STANDARD)
            row += 20;
        GUI_CreateButton(&gui->setperm[i], COL2, row, BUTTON_96x16, show_timerperm_cb, 0x0000, reset_timerperm_cb, (void *)(long)(timer_num | 0x80));
        update_countdown(timer_num);
        int tmpselectable = (Model.timer[timer_num].type < TIMER_COUNTDOWN) ? 2 : 3;
        if (Model.mixer_mode == MIXER_ADVANCED)
            tmpselectable++;
        if(HAS_PERMANENT_TIMER && Model.timer[timer_num].type == TIMER_PERMANENT)
            tmpselectable = 4;
        selectable += tmpselectable;
        COL1 = 270;
        COL2 = 343;
    }
    return selectable;
}

static void _show_page(int page)
{
    (void) page;
    int init_y = 40;

    PAGE_ShowHeader(PAGE_GetName(PAGEID_TIMER));
    
    GUI_CreateScrollable(&gui->scrollable, 0, init_y, LCD_WIDTH, 200,
                     100, NUM_TIMERS / TIMERCOLUMNS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

static void update_countdown(u8 timernum)
{
    
    int cur_row = GUI_ScrollableCurrentRow(&gui->scrollable);

    int idx = timernum - cur_row * TIMERCOLUMNS;
    if (idx < 0 || idx >= TIMERS_PER_PAGE)
        return;
    u8 hide = Model.timer[timernum].type == TIMER_STOPWATCH
              || Model.timer[timernum].type == TIMER_STOPWATCH_PROP
              || Model.timer[timernum].type == TIMER_PERMANENT;
    GUI_SetHidden((guiObject_t *)&gui->start[idx], hide);
    GUI_SetHidden((guiObject_t *)&gui->startlbl[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->start[idx], !hide);

    // Permanent timer  OR Standard Mixer do not have reset command
    hide = Model.timer[timernum].type == TIMER_PERMANENT || Model.mixer_mode == MIXER_STANDARD ;
    GUI_SetHidden((guiObject_t *)&gui->resetsrc[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->resetsrc[idx], !hide);
    GUI_SetHidden((guiObject_t *)&gui->resetlbl[idx], hide);

    hide = Model.timer[timernum].type == TIMER_STOPWATCH
           || Model.timer[timernum].type == TIMER_COUNTDOWN
           || Model.timer[timernum].type == TIMER_STOPWATCH_PROP
           || Model.timer[timernum].type == TIMER_COUNTDOWN_PROP;
    GUI_SetHidden((guiObject_t *)&gui->resetperm[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->resetperm[idx], !hide);
    GUI_SetHidden((guiObject_t *)&gui->setperm[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->setperm[idx], !hide);
    GUI_SetHidden((guiObject_t *)&gui->resetpermlbl[idx], hide);

    GUI_Redraw(&gui->switchlbl[idx]);
}

void reset_timerperm_cb(guiObject_t *obj, const void *data)
{
    long index = (long)data & 0xff;
    if (index & 0x80) {   // set
        PAGE_PushByID(PAGEID_SETTIMER, index & 0x7f);
    } else  {  // reset
        PAGE_ShowResetPermTimerDialog(obj,(void *)(index & 0x7f));
    }
}

#if HAS_RTC
void press_set_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_RemoveAllObjects();
    PAGE_RTCInit(0);
}

const char *show_set_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    snprintf(tempstring, sizeof(tempstring), "%s", _tr("Date / Time"));
    return tempstring;
}

const char *show_time_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    RTC_GetTimeString(tempstring, RTC_GetValue());
    return tempstring;
}

const char *show_date_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    RTC_GetDateStringLong(tempstring, RTC_GetValue());
    return tempstring;
}
#endif
