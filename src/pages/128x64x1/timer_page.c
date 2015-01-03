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

#include "../common/_timer_page.c"

static u16 current_selected = 0;

static unsigned _action_cb(u32 button, unsigned flags, void *data);

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)relrow;
    (void)data;
    /* This is really evil*/
    int idx = gui->scrollable.cur_row;
    int rows = (Model.timer[idx].type < TIMER_COUNTDOWN) ? 2 : 3;
    if (Model.mixer_mode == MIXER_ADVANCED)
        rows++;
#if HAS_PERMANENT_TIMER
    if (Model.timer[idx].type == TIMER_PERMANENT)
        rows = 4;
#endif
    col = (rows + col) % rows;
    switch(col) {
        case 0: return (guiObject_t *)&gui->type;
        case 1: return (guiObject_t *)&gui->src;
        case 2:
#if HAS_PERMANENT_TIMER
            if(Model.timer[idx].type == TIMER_PERMANENT)
                return (guiObject_t *)&gui->resetperm;
#endif
            if(Model.mixer_mode == MIXER_ADVANCED)
                return (guiObject_t *)&gui->resetsrc;
            else
                return (guiObject_t *)&gui->start;
        case 3:
#if HAS_PERMANENT_TIMER
            if(Model.timer[idx].type == TIMER_PERMANENT)
                return (guiObject_t *)&gui->setperm;
#endif
            return (guiObject_t *)&gui->start;
    }
    return 0;
}

/*            Advanced                     Standard
   Row1       Timer                        Timer
   Row2       Switch                       Switch
   Row3       Reset(perm)/ResetSwitch      Reset(perm)/Start
   Row4       Start/Set-to                 Set-to
*/
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    (void)relrow;
    u8 space = LINE_SPACE;
    u8 w = 65;
    u8 x = 55;
    //Row 1
    GUI_CreateLabelBox(&gui->name, 0, y,
            55, LINE_HEIGHT, &DEFAULT_FONT, timer_str_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->type, x, y,
            w, LINE_HEIGHT, &DEFAULT_FONT, toggle_timertype_cb, set_timertype_cb, (void *)(long)absrow);

    //Row 2
    y += space;
    GUI_CreateLabelBox(&gui->switchlbl, 0, y,
            55, LINE_HEIGHT,&DEFAULT_FONT, switch_str_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->src, x, y,
            w, LINE_HEIGHT, &DEFAULT_FONT, toggle_source_cb, set_source_cb, (void *)(long)absrow);
    //Row 3
    y += space;
    /*prem-timer reset */
    GUI_CreateLabelBox(&gui->resetpermlbl, 0, y  ,
            55, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Reset"));
    GUI_CreateButtonPlateText(&gui->resetperm, x+3, y ,
            w-6, LINE_HEIGHT,&DEFAULT_FONT, show_timerperm_cb, 0x0000, reset_timerperm_cb,(void *)(long)absrow);
    if(Model.mixer_mode != MIXER_STANDARD) {
        /* or Reset switch */
    	GUI_CreateLabelBox(&gui->resetlbl, 0, y ,
            55, LINE_HEIGHT,&DEFAULT_FONT, NULL, NULL, _tr("Reset sw"));
    	GUI_CreateTextSelectPlate(&gui->resetsrc, x, y ,
            w, LINE_HEIGHT, &DEFAULT_FONT, toggle_resetsrc_cb, set_resetsrc_cb, (void *)(long)absrow);
	y += space;
    }
    //Row 4
    GUI_CreateLabelBox(&gui->startlbl, 0, y ,
            50, // bug fix: label width and height can't be 0, otherwise, the label couldn't be hidden dynamically
            LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Start"));
    GUI_CreateTextSelectPlate(&gui->start, x, y,
            w, LINE_HEIGHT, &DEFAULT_FONT,NULL, set_start_cb, (void *)(long)absrow);
// don't include this in Devo7e due to memory restrictions
#if HAS_PERMANENT_TIMER
    if(Model.mixer_mode == MIXER_STANDARD)
        y+= space;
    GUI_CreateButtonPlateText(&gui->setperm, x+3, y,
        w-6, LINE_HEIGHT,&DEFAULT_FONT, show_timerperm_cb, 0x0000, reset_timerperm_cb,(void *)(long)(absrow | 0x80));
#endif

    update_countdown(absrow);

    int selectable = (Model.timer[absrow].type < TIMER_COUNTDOWN) ? 2 : 3;
    if (Model.mixer_mode == MIXER_ADVANCED)
        selectable++;
#if HAS_PERMANENT_TIMER
    if (Model.timer[absrow].type == TIMER_PERMANENT)
        selectable = 4;
#endif
    return selectable;
 }

static void _show_page(int page)
{
    (void)page;
    PAGE_ShowHeader(_tr("Timers")); // using the same name as related menu item to reduce language strings
    PAGE_SetActionCB(_action_cb);

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LCD_HEIGHT - HEADER_HEIGHT, NUM_TIMERS, row_cb, getobj_cb, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}

void PAGE_TimerExit()
{
    current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static void update_countdown(u8 idx)
{
    u8 hide = Model.timer[idx].type == TIMER_STOPWATCH
              || Model.timer[idx].type == TIMER_STOPWATCH_PROP
              || Model.timer[idx].type == TIMER_PERMANENT;
    GUI_SetHidden((guiObject_t *)&gui->start, hide);
    GUI_SetHidden((guiObject_t *)&gui->startlbl, hide);
    GUI_SetSelectable((guiObject_t *)&gui->start, !hide);

    // Permanent timer do not have reset command
    hide = Model.timer[idx].type == TIMER_PERMANENT
           || Model.mixer_mode == MIXER_STANDARD ;
    GUI_SetHidden((guiObject_t *)&gui->resetsrc, hide);
    GUI_SetSelectable((guiObject_t *)&gui->resetsrc, !hide);
    GUI_SetHidden((guiObject_t *)&gui->resetlbl, hide);

    hide = Model.timer[idx].type == TIMER_STOPWATCH
           || Model.timer[idx].type == TIMER_STOPWATCH_PROP
           || Model.timer[idx].type == TIMER_COUNTDOWN
           || Model.timer[idx].type == TIMER_COUNTDOWN_PROP;
    GUI_SetHidden((guiObject_t *)&gui->resetperm, hide);
    GUI_SetSelectable((guiObject_t *)&gui->resetperm, !hide);
    GUI_SetHidden((guiObject_t *)&gui->resetpermlbl, hide);
// don't include this in Devo7e due to memory restrictions
#if HAS_PERMANENT_TIMER
    GUI_SetHidden((guiObject_t *)&gui->setperm, hide);
    GUI_SetSelectable((guiObject_t *)&gui->setperm, !hide);
#endif

    GUI_Redraw(&gui->switchlbl);
}

void reset_timerperm_cb(guiObject_t *obj, const void *data)
{
    long index = (long)data & 0xff;
// don't include this in Devo7e due to memory restrictions
#if HAS_PERMANENT_TIMER
    if (index & 0x80) {   // set
        current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
        PAGE_RemoveAllObjects();
        PAGE_SetTimerInit(index & 0x7f);
    } else  // reset
#endif
        PAGE_ShowResetPermTimerDialog(obj,(void *)(index & 0x7f));
}
