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
#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"
#include "main_config.h"
#include "telemetry.h"

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
    (void)bar_cb;
    (void)page;
    int i;
    u16 x, y, w, h;
    PAGE_SetModal(0);
    PAGE_SetActionCB(_action_cb);
    next_scan = CLOCK_getms()+BATTERY_SCAN_MSEC;

    //mp->optsObj = GUI_CreateIcon(0, 0, &icons[ICON_OPTIONS], press_icon2_cb, (void *)0);
    //if(! MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
    //    GUI_CreateIcon(32, 0, &icons[ICON_MODELICO], press_icon2_cb, (void *)1);

    mp->nameObj = GUI_CreateLabelBox(0, 1, //64, 12,
            0, 0, &SMALL_FONT, NULL, NULL, Model.name);

    // Logo label
    //GUI_CreateLabelBox(10, 12, 0, 0, &SMALL_FONT, NULL, NULL, "Devil 10");

    //heli/plane Icon
    struct LabelDesc desc = DEFAULT_FONT;
    desc.font_color = 1;
    desc.fill_color = 1;
    if (MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
        GUI_CreateRect(x, y, w, h, &desc);
        //GUI_CreateImageOffset(x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), NULL, (void *)1);

    for(i = 0; i < 6; i++) {
        mp->trims[i] = Model.trims[i].value;
        if (MAINPAGE_GetWidgetLoc(TRIM1+i, &x, &y, &w, &h))
            mp->trimObj[i] = GUI_CreateBarGraph(x, y, w, h, -100, 100, i & 0x02 ? TRIM_INVHORIZONTAL : TRIM_VERTICAL, trim_cb, &Model.trims[i].value);
        else
            mp->trimObj[i] = NULL;
    }

    // show thro , timer1 ,timer 2
    for(i = 0; i < 8; i++) {
        if (MAINPAGE_GetWidgetLoc(BOX1+i, &x, &y, &w, &h)) { // i = 0, 1, 2
            mp->boxval[i] = get_boxval(Model.pagecfg.box[i]);
            mp->boxObj[i] = GUI_CreateLabelBox(x, y, w, h,
                                //get_box_font(i, Model.pagecfg.box[i] <= 2 && mp->boxval[i] < 0),
                                &TINY_FONT,
                                show_box_cb, NULL,
                                (void *)((long)Model.pagecfg.box[i]));
        } else { // i = 3 - 7
            mp->boxval[i] = 0;
            mp->boxObj[i] = NULL;
        }
    }
    //Battery
    mp->battery = PWR_ReadVoltage();
    mp->battObj = GUI_CreateLabelBox(85,1, 40, 8, &TINY_FONT,  voltage_cb, NULL, NULL);
    //TxPower
    GUI_CreateLabelBox(85, 12,  //54,1,
            40, 8,&TINY_FONT, _power_to_string, NULL, NULL);
}

void PAGE_MainEvent()
{
    int i;
    if (PAGE_GetModal())
        return;
    for(i = 0; i < 6; i++) {
        if (! mp->trimObj[i])
            continue;
        if (mp->trims[i] != Model.trims[i].value) {
            mp->trims[i] = Model.trims[i].value;
            GUI_Redraw(mp->trimObj[i]);
        }
    }
    for(i = 0; i < 8; i++) {
        if (! mp->boxObj[i])
            continue;
        s32 val = get_boxval(Model.pagecfg.box[i]);
        if (Model.pagecfg.box[i] <= 2) {
            if ((val >= 0 && mp->boxval[i] < 0) || (val < 0 && mp->boxval[i] >= 0)) {
                //Timer
                //GUI_SetLabelDesc(mp->boxObj[i], get_box_font(i, val < 0));
                mp->boxval[i] = val;
                GUI_Redraw(mp->boxObj[i]);
            } else if (mp->boxval[i] / 1000 != val /1000) {
                mp->boxval[i] = val;
                GUI_Redraw(mp->boxObj[i]);
            }
        } else if (mp->boxval[i] != val) {
            mp->boxval[i] = val;
            GUI_Redraw(mp->boxObj[i]);
        }
    }
    if (CLOCK_getms() > next_scan)  {  // don't need to check battery too frequently, to avoid blink of the battery label
        next_scan = CLOCK_getms() + BATTERY_SCAN_MSEC;
        s16 batt = PWR_ReadVoltage();
        if (batt / 10 != mp->battery / 10 && batt / 10 != mp->battery / 10 + 1) {
            mp->battery = batt;
            GUI_Redraw(mp->battObj);
        }
    }
}

void PAGE_MainExit()
{
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_ENTER)) {
        PAGE_ChangeByName("MainMenu", 0);
    }
    else if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_UP)) {
        mp->ignore_release = 1;
        TIMER_StartStop(0);
        TIMER_StartStop(1);
    } else if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_DOWN)) {
        TIMER_Reset(0);
        TIMER_Reset(1);;
    }
    else {
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
