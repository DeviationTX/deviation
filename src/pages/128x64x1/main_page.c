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

//#ifdef DEBUG_KEY_INPUT  // To debug button press codes in real devo10

static struct main_page * const mp = &pagemem.u.main_page;
const char *show_box_cb(guiObject_t *obj, const void *data);
const char *voltage_cb(guiObject_t *obj, const void *data);
static s16 trim_cb(void * data);
//void press_icon_cb(guiObject_t *obj, s8 press_type, const void *data);
//void press_icon2_cb(guiObject_t *obj, const void *data);
//void press_box_cb(guiObject_t *obj, s8 press_type, const void *data);
static u8 action_cb(u32 button, u8 flags, void *data);
const char *power_to_string();
static s32 get_boxval(u8 idx);
static s16 last_battery = 0;

/*
 * Main page
 * KEY_UP: Press once to start timers, press again to stop timers
 * KEY_DOWN: Press to rest timers
 * KEY_ENT: enter the main menu page
 */
void PAGE_MainInit(int page)
{
    (void)page;
    int i;
    u16 x, y, w, h;
    PAGE_SetModal(0);
    PAGE_SetActionCB(action_cb);

    //mp->optsObj = GUI_CreateIcon(0, 0, &icons[ICON_OPTIONS], press_icon2_cb, (void *)0);
    //if(! MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
    //    GUI_CreateIcon(32, 0, &icons[ICON_MODELICO], press_icon2_cb, (void *)1);

    mp->nameObj = GUI_CreateLabelBox(64, 12, 70, 10, &SMALL_FONT, NULL, NULL, Model.name);

    // Logo label
    GUI_CreateLabelBox(10, 12, 0, 0, &SMALL_FONT, NULL, NULL, "Devil 10");

    //heli/plane Icon
    if (MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
        GUI_CreateImageOffset(x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), NULL, (void *)1);

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
    GUI_CreateLabelBox(54,1, 0, 0,&TINY_FONT, power_to_string, NULL, NULL);
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
    s16 batt = PWR_ReadVoltage();
    if (batt / 10 != mp->battery / 10 && batt / 10 != mp->battery / 10 + 1) {
        mp->battery = batt;
        GUI_Redraw(mp->battObj);
    }
#ifdef DEBUG_KEY_INPUT
    GUI_Redraw(mp->battObj); //  remove this line
#endif
}

void PAGE_MainExit()
{
}

static u8 action_cb(u32 button, u8 flags, void *data)
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

s32 get_boxval(u8 idx)
{
    if (idx <= NUM_TIMERS)
        return TIMER_GetValue(idx - 1);
    if(idx - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_GetValue(idx - NUM_TIMERS);
    return RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY));
}

const char *show_box_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    if (idx <= NUM_TIMERS) {
        TIMER_SetString(mp->tmpstr, TIMER_GetValue(idx - 1));
    } else if(idx - NUM_TIMERS <= NUM_TELEM) {
        TELEMETRY_GetValueStr(mp->tmpstr, idx - NUM_TIMERS);
    } else {
        sprintf(mp->tmpstr, "%3d%%", RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY)));
    }
    return mp->tmpstr;
}

const char *voltage_cb(guiObject_t *obj, const void *data) {
    static u8 voltageClear = 0;
    (void)obj;
    (void)data;
    if (mp->battery - last_battery < 0 || mp->battery  - last_battery > 100){
        last_battery = mp->battery; // to avoid blinking in battery display

    }
    sprintf(mp->tmpstr, "%2d.%02dv", last_battery / 1000, (last_battery % 1000) / 10);
    if (mp->battery < Transmitter.batt_alarm) {  // let the voltage label flashed when low voltage
        voltageClear = !voltageClear;
        if (voltageClear) {
            mp->tmpstr[0] = 0;
        }
    }
#ifdef DEBUG_KEY_INPUT
    int i = ScanButtons();
    sprintf(mp->tmpstr, "%x", i);
#endif
    return mp->tmpstr;
}

s16 trim_cb(void * data)
{
    s8 *trim = (s8 *)data;
    return *trim;
}

/* s16 bar_cb(void * data)
{
    u8 idx = (long)data;
    return MIXER_GetChannel(idx-1, APPLY_SAFETY);
}

void press_icon_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if(press_type == -1) {
        if ((long)data == 0) {
            PAGE_SetSection(SECTION_OPTIONS);
        } else if ((long)data == 1) {
            PAGE_SetSection(SECTION_MODEL);
        } else {
            PAGE_SetModal(1);
            PAGE_MainExit();
            MODELPage_ShowLoadSave(0, PAGE_MainInit);
        }
    }
}

void press_icon2_cb(guiObject_t *obj, const void *data)
{
    press_icon_cb(obj, -1, data);
}

void press_box_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    if (idx > 2)
        return;
    if(press_type == -1 && ! mp->ignore_release) 
        TIMER_StartStop(idx-1);
    mp->ignore_release = 0;
    if(press_type > 0) {
        TIMER_Reset(idx-1);
        mp->ignore_release = 1;
    }
}*/

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
const char *power_to_string()
{
    return RADIO_TX_POWER_VAL[Model.tx_power];
}
