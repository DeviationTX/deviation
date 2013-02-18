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
#define MAX_TIMER_PAGE 1 // 2 - 1 
static void _draw_body();
guiObject_t *firstObj;
s8 timer_page_num ;

static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
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


static void _show_page()
{
    PAGE_SetModal(0);
    firstObj = NULL;
    timer_page_num = 0 ;

    if (Model.mixer_mode == MIXER_STANDARD)
        PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_TIMER), MODELMENU_Show);
    else
        PAGE_ShowHeader(PAGE_GetName(PAGEID_TIMER));
    
    GUI_CreateScrollbar(&gui->scrollbar, 304, 32, 208, MAX_TIMER_PAGE + 1, NULL, scroll_cb, NULL);
    GUI_SetScrollbar(&gui->scrollbar,timer_page_num);
    _draw_body();
}

static void _draw_body() {
    if (firstObj) {
        GUI_RemoveHierObjects(firstObj);
        firstObj = NULL;
    }
    
    for (u8 i = timer_page_num * 2; i < NUM_TIMERS && i < timer_page_num * 2 + 2; i++) {
	u8 y = 20 ;
        u8 x = 48 + i%2 * 96;
        //Row 1
	if(!i%2) 
            firstObj = GUI_CreateLabel(&gui->timer[i], y, x, timer_str_cb, DEFAULT_FONT, (void *)(long)i);
	else
	    GUI_CreateLabel(&gui->timer[i], y, x, timer_str_cb, DEFAULT_FONT, (void *)(long)i);
		
        GUI_CreateTextSelect(&gui->type[i], y+63 , x, TEXTSELECT_96, toggle_timertype_cb, set_timertype_cb, (void *)(long)i);
        //Row 2
	GUI_CreateLabel(&gui->switchlbl[i], y, x+22, NULL, DEFAULT_FONT, _tr("Switch:"));
        GUI_CreateTextSelect(&gui->src[i],  y+63, x+22, TEXTSELECT_96, toggle_source_cb, set_source_cb, (void *)(long)i);
        //Row 3
        GUI_CreateLabelBox(&gui->resetlbl[i], y, x+44, 50, 12, &DEFAULT_FONT, NULL, NULL, _tr("Reset sw:"));
        GUI_CreateTextSelect(&gui->resetsrc[i],  y+63, x+44, TEXTSELECT_96, toggle_resetsrc_cb, set_resetsrc_cb, (void *)(long)i);
         /* or Reset Perm timer*/
	GUI_CreateLabelBox(&gui->resetpermlbl[i], y, x+44, 50, 12, &DEFAULT_FONT, NULL, NULL, _tr("Reset"));
	GUI_CreateButton(&gui->resetperm[i], y+63, x+44, TEXTSELECT_96, show_timerperm_cb, 0x0000, reset_timerperm_cb, (void *)(long)i);
        //Row 4
        GUI_CreateLabelBox(&gui->startlbl[i], y, x+66, 50, 12, &DEFAULT_FONT, NULL, NULL, _tr("Start:"));
        GUI_CreateTextSelect(&gui->start[i], y+63, x+66, TEXTSELECT_96, NULL, set_start_cb, (void *)(long)i);
        update_countdown(i);
    }
}

static void update_countdown(u8 idx)
{
    u8 hide = Model.timer[idx].type == TIMER_STOPWATCH || Model.timer[idx].type == TIMER_PERMANENT;
    GUI_SetHidden((guiObject_t *)&gui->start[idx], hide);
    GUI_SetHidden((guiObject_t *)&gui->startlbl[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->start[idx], !hide);

    // Permanent timer do not have reset command
    hide = Model.timer[idx].type == TIMER_PERMANENT;
    GUI_SetHidden((guiObject_t *)&gui->resetsrc[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->resetsrc[idx], !hide);
    GUI_SetHidden((guiObject_t *)&gui->resetlbl[idx], hide);

    hide = Model.timer[idx].type == TIMER_STOPWATCH || Model.timer[idx].type == TIMER_COUNTDOWN;
    GUI_SetHidden((guiObject_t *)&gui->resetperm[idx], hide);
    GUI_SetSelectable((guiObject_t *)&gui->resetperm[idx], !hide);
    GUI_SetHidden((guiObject_t *)&gui->resetpermlbl[idx], hide);
}
