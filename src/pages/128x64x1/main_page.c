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
//#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

#ifndef OVERRIDE_PLACEMENT
#define TGLICO_LoadFonts() (void)1
enum {
     VTRIM_W      =  4,
     VTRIM_H      = 49,
     HTRIM_W      = 49,
     HTRIM_H      =  4,
     MODEL_ICO_W  = 52,
     MODEL_ICO_H  = 36,
     BOX_W        = 48,
     SMALLBOX_H   = 10,
     BIGBOX_H     = 14,
     GRAPH_W      = (VTRIM_W),
     GRAPH_H      = (VTRIM_H / 2),
     BATTERY_W    = 26,
     BATTERY_H    = 6,
     TXPOWER_W    = 26,
     TXPOWER_H    = 6,
//
    MODEL_NAME_X  = 0,
    MODEL_NAME_Y  = 1,
};

#endif //OVERRIDE_PLACEMENT
#define press_icon_cb NULL
#define press_box_cb NULL

#include "../common/_main_page.c"

static const int BATTERY_SCAN_MSEC = 2000; // refresh battery for every 2sec to avoid its label blinking
static u32 next_scan=0;

/*
 * Main page
 * KEY_UP: Press once to start timers, press again to stop timers
 * KEY_DOWN: Press to rest timers
 * KEY_ENT: enter the main menu page
 */
void PAGE_MainInit(int page)
{
    (void)page;
    (void)bar_cb;
    TGLICO_LoadFonts();
    memset(mp, 0, sizeof(struct main_page));// Bug fix: must initialize this structure to avoid unpredictable issues in the PAGE_MainEvent
    memset(gui, 0, sizeof(struct mainpage_obj));
    PAGE_SetModal(0);
    PAGE_SetActionCB(_action_cb);
    PAGE_RemoveAllObjects();
    next_scan = CLOCK_getms()+BATTERY_SCAN_MSEC;

    GUI_CreateLabelBox(&gui->name, MODEL_NAME_X, MODEL_NAME_Y, //64, 12,
            0, 0, &SMALL_FONT, NULL, NULL, Model.name);


    show_elements();
    //Battery
    mp->battery = PWR_ReadVoltage();
}

static void _check_voltage(guiLabel_t *obj)
{
    if (CLOCK_getms() > next_scan)  {  // don't need to check battery too frequently, to avoid blink of the battery label
        next_scan = CLOCK_getms() + BATTERY_SCAN_MSEC;
        s16 batt = PWR_ReadVoltage();
        if (batt < Transmitter.batt_alarm) {
            obj->desc.style = LABEL_INVERTED;
            GUI_Redraw(obj);
        }
        if (batt / 10 != mp->battery / 10 && batt / 10 != mp->battery / 10 + 1) {
            mp->battery = batt;
            GUI_Redraw(obj);
        }
    }
}

void PAGE_MainExit()
{
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    u8 i;
    if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_ENTER)) {
        u8 page = (0 << 4) | MENUTYPE_MAINMENU;
        PAGE_ChangeByID(PAGEID_MENU, page);
    } else if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
	for ( i=0; i< NUM_TIMERS; i++) 
            TIMER_StartStop(i);
    } else if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_LEFT)) {
        for ( i=0; i< NUM_TIMERS; i++)
            TIMER_Reset(i);
    } else if (! PAGE_QuickPage(button, flags, data)) {
        MIXER_UpdateTrim(button, flags, data);
    }
    return 1;
}
