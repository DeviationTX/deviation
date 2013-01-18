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
#include "../common/main_config.h"
#include "telemetry.h"

#include "../common/_main_page.c"

static const char *_power_to_string();
static u8 _action_preview_cb(u32 button, u8 flags, void *data);
static const char *show_toggle_cb(guiObject_t *obj, const void *data);

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
    int i;
    u16 x, y, w, h;
    memset(mp, 0, sizeof(struct main_page));// Bug fix: must initialize this structure to avoid unpredictable issues in the PAGE_MainEvent
    memset(gui, 0, sizeof(struct mainpage_obj));
    PAGE_SetModal(0);
    if (page == 1)
        PAGE_SetActionCB(_action_preview_cb);
    else
        PAGE_SetActionCB(_action_cb);
    GUI_RemoveAllObjects();
    next_scan = CLOCK_getms()+BATTERY_SCAN_MSEC;

    //mp->optsObj = GUI_CreateIcon(0, 0, &icons[ICON_OPTIONS], press_icon2_cb, (void *)0);
    //if(! MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
    //    GUI_CreateIcon(32, 0, &icons[ICON_MODELICO], press_icon2_cb, (void *)1);

    GUI_CreateLabelBox(&gui->name, 0, 1, //64, 12,
            0, 0, &SMALL_FONT, NULL, NULL, Model.name);

    // Logo label
    //GUI_CreateLabelBox(10, 12, 0, 0, &SMALL_FONT, NULL, NULL, "Devil 10");

    //heli/plane Icon
    if (MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
        GUI_CreateImageOffset(&gui->icon, x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), NULL, (void *)1);

    for(i = 0; i < 6; i++) {
        mp->trims[i] = Model.trims[i].value;
        if (MAINPAGE_GetWidgetLoc(TRIM1+i, &x, &y, &w, &h)) {
            GUI_CreateBarGraph(&gui->trim[i], x, y, w, h, -100, 100, i & 0x02 ? TRIM_INVHORIZONTAL : TRIM_VERTICAL, trim_cb, &Model.trims[i].value);
        }
    }

    // show thro , timer1 ,timer 2
    for(i = 0; i < 8; i++) {
        if (MAINPAGE_GetWidgetLoc(BOX1+i, &x, &y, &w, &h)) { // i = 0, 1, 2
            mp->boxval[i] = get_boxval(Model.pagecfg.box[i]);
            GUI_CreateLabelBox(&gui->box[i], x, y, w, h,
                                get_box_font(i, Model.pagecfg.box[i] <= 2 && mp->boxval[i] < 0),
                                show_box_cb, NULL,
                                (void *)((long)Model.pagecfg.box[i]));
        } else { // i = 3 - 7
            mp->boxval[i] = 0;
        }
    }

    for(i = 0; i < 4; i++) {
        if (MAINPAGE_GetWidgetLoc(TOGGLE1+i, &x, &y, &w, &h)) {
            u8 old_color = TINY_FONT.outline_color;
            TINY_FONT.outline_color = 0xffff;
            u8 x1 = x + (w +2)* i;
            if(! Model.pagecfg.toggle[i])
                LCD_FillRect(x1+1, y, w, h, 0x0);  // clear the area
            else {
                GUI_CreateLabelBox(&gui->toggle[i], x1 + 1 , y, w, h, &TINY_FONT, show_toggle_cb, NULL, (void *)(long)i);
            }
            TINY_FONT.outline_color = old_color;
        }
    }
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
    if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_ENTER)) {
        u8 page = (0 << 4) | MENUTYPE_MAINMENU;
        PAGE_ChangeByID(PAGEID_MENU, page);
    } else if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
        TIMER_StartStop(0);
        TIMER_StartStop(1);
    } else if ((flags & BUTTON_PRESS) && CHAN_ButtonIsPressed(button, BUT_LEFT)) {
        TIMER_Reset(0);
        TIMER_Reset(1);;
    } else if (! PAGE_QuickPage(button, flags, data)) {
        MIXER_UpdateTrim(button, flags, data);
    }
    return 1;
}

static u8 _action_preview_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MAINCFG, -1);  // go back to the main config page
        }
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

static const char *show_toggle_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 i = (long)data;
    sprintf(mp->tmpstr, "%d", i + 1);
    return mp->tmpstr;
}
