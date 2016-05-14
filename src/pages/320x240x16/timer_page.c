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
    MAX_TIMER_PAGE = ((NUM_TIMERS - 1) / (2 * TIMERCOLUMNS)),
};

static void _draw_body();
guiObject_t *firstObj;
s8 timer_page_num ;

void press_set_cb(guiObject_t *obj, const void *data);
const char *show_set_cb(guiObject_t *obj, const void *data);
const char *show_time_cb(guiObject_t *obj, const void *data);
const char *show_date_cb(guiObject_t *obj, const void *data);

static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)pos;
    (void)parent;
    (void)data;
    s8 newpos = (s8)timer_page_num+ (direction > 0 ? 1 : -1);
    if (newpos < 0)
        newpos = 0;
    else if (newpos > MAX_TIMER_PAGE)
        newpos = MAX_TIMER_PAGE;
    if (newpos != timer_page_num) {
        timer_page_num = newpos;
        _draw_body();
    }
    return timer_page_num;
}


static void _show_page(int page)
{
    PAGE_SetModal(0);
    firstObj = NULL;
    timer_page_num = (LCD_WIDTH == 480 ? 0 : page);  // ignore for big screen because everything is on one page

#if HAS_STANDARD_GUI
    if (Model.mixer_mode == MIXER_STANDARD)
        PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_TIMER), MODELMENU_Show);
    else
#endif
        PAGE_ShowHeader(PAGE_GetName(PAGEID_TIMER));
    
    GUI_CreateScrollbar(&gui->scrollbar, LCD_WIDTH-16, 32, LCD_HEIGHT-32, MAX_TIMER_PAGE + 1, NULL, scroll_cb, NULL);
    GUI_SetScrollbar(&gui->scrollbar,timer_page_num);
    _draw_body();
}

/*            Advanced                     Standard
   Row1       Timer                        Timer
   Row2       Switch                       Switch
   Row3       Reset(perm)/ResetSwitch      Reset(perm)/Start
   Row4       Start/Set-to                 Set-to
*/
static void _draw_body() {
    if (firstObj) {
        GUI_RemoveHierObjects(firstObj);
        firstObj = NULL;
    }
    int COL1 = 30;
    int COL2 = 103;
    for (u8 i = timer_page_num * 2 * TIMERCOLUMNS; i < NUM_TIMERS  && i < (timer_page_num + 1) * 2 * TIMERCOLUMNS; i++) {
        if (TIMERCOLUMNS == 1 || (TIMERCOLUMNS == 2 && i < timer_page_num * 2 * TIMERCOLUMNS + TIMERCOLUMNS)) {
            COL1 = 30;
            COL2 = 130;
        } else {
            COL1 = 270;
            COL2 = 343;
        }
        int row = 48 + i%2 * 100;
        //Row 1
        if(firstObj == NULL)
            firstObj = GUI_CreateLabelBox(&gui->timer[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT,timer_str_cb, NULL, (void *)(long)i);
        else
            GUI_CreateLabelBox(&gui->timer[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, timer_str_cb, NULL, (void *)(long)i);

        GUI_CreateTextSelect(&gui->type[i], COL2, row, TEXTSELECT_96, toggle_timertype_cb, set_timertype_cb, (void *)(long)i);
        //Row 2
        row+=20;
#if 0 //HAS_RTC
        //GUI_CreateLabelBox(&gui->timelbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL,NULL,_tr("Time:"));
        GUI_CreateLabelBox(&gui->timevallbl[i], COL2+30, row, 96, 18, &DEFAULT_FONT, show_time_cb, NULL, (void *)(long)i);
#endif
        GUI_CreateLabelBox(&gui->switchlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, switch_str_cb, NULL, (void *)(long)i);
        GUI_CreateTextSource(&gui->src[i],  COL2, row, TEXTSELECT_96, toggle_source_cb, set_source_cb, set_input_source_cb, (void *)(long)i);
        //Row 3
        row+=20;
        /* Reset Perm timer*/
        GUI_CreateLabelBox(&gui->resetpermlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Reset"));
        GUI_CreateButton(&gui->resetperm[i], COL2, row, BUTTON_96x16, show_timerperm_cb, 0x0000, reset_timerperm_cb, (void *)(long)i);
        if(Model.mixer_mode != MIXER_STANDARD) {
            /* or Reset switch */
            GUI_CreateLabelBox(&gui->resetlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Reset sw"));
            GUI_CreateTextSource(&gui->resetsrc[i],  COL2, row, TEXTSELECT_96, toggle_resetsrc_cb, set_resetsrc_cb, set_input_rstsrc_cb, (void *)(long)i);
            row+=20;
        }
        //Row 4
        GUI_CreateLabelBox(&gui->startlbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Start"));
        GUI_CreateTextSelect(&gui->start[i], COL2, row, TEXTSELECT_96, NULL, set_start_cb, (void *)(long)i);
        if(Model.mixer_mode == MIXER_STANDARD)
            row += 20;
        GUI_CreateButton(&gui->setperm[i], COL2, row, BUTTON_96x16, show_timerperm_cb, 0x0000, reset_timerperm_cb, (void *)(long)(i | 0x80));
#if 0 //HAS_RTC
        // date label and set date/time button
        GUI_CreateLabelBox(&gui->datelbl[i], COL1, row, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Date:"));
        GUI_CreateLabelBox(&gui->datevallbl[i], COL2+20, row, 96, 18, &DEFAULT_FONT, show_date_cb, NULL, (void *)(long)i);
        GUI_CreateLabelBox(&gui->setlbl[i], COL1, row+20, COL2-COL1, 18, &DEFAULT_FONT, NULL, NULL, _tr("Set:"));
        GUI_CreateButton(&gui->set[i], COL2, row+20, BUTTON_96x16, show_set_cb, 0x0000, press_set_cb, (void *)(long)i);
#endif
        update_countdown(i);
    }
}

static void update_countdown(u8 idx)
{
    u8 hide = Model.timer[idx].type == TIMER_STOPWATCH
              || Model.timer[idx].type == TIMER_STOPWATCH_PROP
              || Model.timer[idx].type == TIMER_PERMANENT;
    GUI_SetHidden((guiObject_t *)&gui->start[idx], hide);
    GUI_SetHidden((guiObject_t *)&gui->startlbl[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->start[idx], !hide);

    // Permanent timer  OR Standard Mixer do not have reset command
    hide = Model.timer[idx].type == TIMER_PERMANENT || Model.mixer_mode == MIXER_STANDARD ;
    GUI_SetHidden((guiObject_t *)&gui->resetsrc[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->resetsrc[idx], !hide);
    GUI_SetHidden((guiObject_t *)&gui->resetlbl[idx], hide);

    hide = Model.timer[idx].type == TIMER_STOPWATCH
           || Model.timer[idx].type == TIMER_COUNTDOWN
           || Model.timer[idx].type == TIMER_STOPWATCH_PROP
           || Model.timer[idx].type == TIMER_COUNTDOWN_PROP;
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
        PAGE_RemoveAllObjects();
        PAGE_SetTimerInit(index & 0x7f);
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
