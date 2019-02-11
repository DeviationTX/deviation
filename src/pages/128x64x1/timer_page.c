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
    LABEL_X        = 0,
    LABEL_WIDTH    = 57,
    TEXTSEL_X      = 58,
    TEXTSEL_WIDTH  = 65,
    RESET_X        = 61,
    RESET_WIDTH    = 59,
    START_WIDTH    = 57,
};
#endif //OVERRIDE_PLACEMENT

#include "../common/_timer_page.c"

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
    int y_top = y;
    void * lbl_str = NULL;
    void * lbl_data = NULL;
    void * lbl_obj = NULL;
    int w = LABEL_WIDTH;

    // Col 1
    for (int i = 0; i < 4; i++) {
        switch (i) {
            case 0:
                lbl_obj = &gui->name;
                lbl_str = timer_str_cb; lbl_data = (void *)(long)absrow;
                break;
            case 1:
                lbl_obj = &gui->switchlbl;
                lbl_str = switch_str_cb; lbl_data = (void *)(long)absrow;
                break;
            case 2:
                lbl_obj = &gui->resetpermlbl;
                lbl_str = GUI_Localize;
                lbl_data = _tr_noop("Reset");
                if (Model.mixer_mode != MIXER_STANDARD) {
                    lbl_obj = &gui->resetlbl; lbl_data = _tr_noop("Reset sw");
                }
                break;
            case 3:
                w = START_WIDTH;  // bug fix: label width and height can't be 0, otherwise, the label couldn't be hidden dynamically
                lbl_obj = &gui->startlbl;
                lbl_str = GUI_Localize;
                lbl_data = _tr_noop("Start");
                break;
        }
        GUI_CreateLabelBox(lbl_obj, LABEL_X, y, w, LINE_HEIGHT, &LABEL_FONT, lbl_str, NULL, lbl_data);
        y += space;
        if ((i == 2) && (Model.mixer_mode == MIXER_STANDARD))
            y -= space;
    }
    y = y_top;

    // Col 2
    // Row 1
    GUI_CreateTextSelectPlate(&gui->type, TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, set_timertype_cb, (void *)(long)absrow);
    // Row 2
    y += space;
    GUI_CreateTextSourcePlate(&gui->src, TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, toggle_source_cb, set_source_cb, set_input_source_cb, (void *)(long)absrow);
    // Row 3
    y += space;
    /*prem-timer reset */
    GUI_CreateButtonPlateText(&gui->resetperm, RESET_X, y,
            RESET_WIDTH, LINE_HEIGHT, &BUTTON_FONT, show_timerperm_cb, reset_timerperm_cb,(void *)(long)absrow);
    if(Model.mixer_mode != MIXER_STANDARD) {
        /* or Reset switch */
    	GUI_CreateTextSourcePlate(&gui->resetsrc, TEXTSEL_X, y ,
            TEXTSEL_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, toggle_resetsrc_cb, set_resetsrc_cb, set_input_rstsrc_cb, (void *)(long)absrow);
	y += space;
    }
    // Row 4
    GUI_CreateTextSelectPlate(&gui->start, TEXTSEL_X, y,
            TEXTSEL_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, set_start_cb, (void *)(long)absrow);
// don't include this in Devo7e due to memory restrictions
#if HAS_PERMANENT_TIMER
    if(Model.mixer_mode == MIXER_STANDARD)
        y+= space;
    GUI_CreateButtonPlateText(&gui->setperm, RESET_X, y,
        RESET_WIDTH, LINE_HEIGHT, &BUTTON_FONT, show_timerperm_cb, reset_timerperm_cb,(void *)(long)(absrow | 0x80));
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
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TIMER)); // using the same name as related menu item to reduce language strings

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LCD_HEIGHT - HEADER_HEIGHT, NUM_TIMERS, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
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
        PAGE_PushByID(PAGEID_SETTIMER, index & 0x7f);
    } else  // reset
#endif
        PAGE_ShowResetPermTimerDialog(obj,(void *)(index & 0x7f));
}
