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

#define VTRIM_W       4
#define VTRIM_H      49
#define HTRIM_W      49
#define HTRIM_H       4
#define MODEL_ICO_W  52
#define MODEL_ICO_H  36
#define BOX_W        48
#define SMALLBOX_H    9
#define BIGBOX_H      14
#define GRAPH_W      (VTRIM_W)
#define GRAPH_H      (VTRIM_H / 2)

#define press_icon_cb NULL
#define press_box_cb NULL

#include "../common/_main_page.c"

static const char *_power_to_string();

#define BATTERY_SCAN_MSEC 2000 // refresh battery for every 2sec to avoid its label blinking
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
    memset(mp, 0, sizeof(struct main_page));// Bug fix: must initialize this structure to avoid unpredictable issues in the PAGE_MainEvent
    memset(gui, 0, sizeof(struct mainpage_obj));
    PAGE_SetModal(0);
    PAGE_SetActionCB(_action_cb);
    PAGE_RemoveAllObjects();
    next_scan = CLOCK_getms()+BATTERY_SCAN_MSEC;

    GUI_CreateLabelBox(&gui->name, 0, 1, //64, 12,
            0, 0, &SMALL_FONT, NULL, NULL, Model.name);


    show_elements();
    //Battery
    mp->battery = PWR_ReadVoltage();
    GUI_CreateLabelBox(&gui->battery, 88 ,1, 40, 7, &TINY_FONT,  voltage_cb, NULL, NULL);
    //TxPower
    GUI_CreateLabelBox(&gui->power, 88, 12,  //54,1,
            40, 8,&TINY_FONT, _power_to_string, NULL, NULL);
}

static void _check_voltage()
{
    if (CLOCK_getms() > next_scan)  {  // don't need to check battery too frequently, to avoid blink of the battery label
        next_scan = CLOCK_getms() + BATTERY_SCAN_MSEC;
        s16 batt = PWR_ReadVoltage();
        if (batt < Transmitter.batt_alarm) {
            enum LabelType oldStyle = TINY_FONT.style;  // bug fix
            TINY_FONT.style = LABEL_BLINK;
            GUI_SetLabelDesc(&gui->battery, &TINY_FONT);
            GUI_Redraw(&gui->battery);
            TINY_FONT.style = oldStyle;
        }
        if (batt / 10 != mp->battery / 10 && batt / 10 != mp->battery / 10 + 1) {
            mp->battery = batt;
            GUI_Redraw(&gui->battery);
        }
    }
}

void PAGE_MainExit()
{
}

static u8 _action_cb(u32 button, u8 flags, void *data)
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

/**
 * Below are defined in the common.h
 *  TXPOWER_100uW,  // -10db
 *  TXPOWER_300uW, // -5db
 *  TXPOWER_1mW, //0db
 *  TXPOWER_3mW, // 5db
 *  TXPOWER_10mW, // 10db
 *  TXPOWER_30mW, // 15db
 *  TXPOWER_100mW, // 20db
 *  TXPOWER_150mW, // 22db
 */
static const char *_power_to_string()
{
    return RADIO_TX_POWER_VAL[Model.tx_power];
}
