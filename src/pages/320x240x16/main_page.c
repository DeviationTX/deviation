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
#include "../common/main_config.h"
#include "telemetry.h"

#include "../common/_main_page.c"

void PAGE_MainInit(int page)
{
    (void)page;
    int i;
    u16 x, y, w, h;
    memset(mp, 0, sizeof(struct main_page));
    memset(gui, 0, sizeof(*gui));
    PAGE_SetModal(0);
    BUTTON_RegisterCallback(&mp->action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, _action_cb, NULL);

    GUI_CreateIcon(&gui->optico, 0, 0, &icons[ICON_OPTIONS], press_icon2_cb, (void *)0);
    if(! MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
        GUI_CreateIcon(&gui->model.ico, 32, 0, &icons[ICON_MODELICO], press_icon2_cb, (void *)1);

    GUI_CreateLabelBox(&gui->name, 96, 8, 128, 24, &MODELNAME_FONT,
                                      NULL, press_icon_cb, Model.name);

    //Icon
    if (MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
        GUI_CreateImageOffset(&gui->model.img, x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), press_icon_cb, (void *)1);

    for(i = 0; i < 6; i++) {
        mp->trims[i] = Model.trims[i].value;
        if (MAINPAGE_GetWidgetLoc(TRIM1+i, &x, &y, &w, &h))
            GUI_CreateBarGraph(&gui->trim[i], x, y, w, h, -100, 100, i & 0x02 ? TRIM_INVHORIZONTAL : TRIM_VERTICAL, trim_cb, &Model.trims[i].value);
    }
    for(i = 0; i < 8; i++) {
        if (MAINPAGE_GetWidgetLoc(BOX1+i, &x, &y, &w, &h)) {
            mp->boxval[i] = get_boxval(Model.pagecfg.box[i]);
            int font = ((Model.pagecfg.box[i] <= NUM_TIMERS && mp->boxval[i] < 0) ||
                        ((u8)(Model.pagecfg.box[i] - NUM_TIMERS - 1) < NUM_TELEM && Telemetry.time[0] == 0));
            GUI_CreateLabelBox(&gui->box[i], x, y, w, h,
                                get_box_font(i, font),
                                show_box_cb, press_box_cb,
                                (void *)((long)Model.pagecfg.box[i]));
        } else {
            mp->boxval[i] = 0;
        }
    }
    for(i = 0; i < 8; i++) {
        if (i >= NUM_OUT_CHANNELS || i >= Model.num_channels)
            break;
        if (MAINPAGE_GetWidgetLoc(BAR1+i, &x, &y, &w, &h)) {
            mp->barval[i] = MIXER_GetChannel(Model.pagecfg.bar[i]-1, APPLY_SAFETY);
            GUI_CreateBarGraph(&gui->bar[i], x, y, w, h, CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                                               bar_cb, (void *)((long)Model.pagecfg.bar[i]));
        } else {
            mp->barval[i] = 0;
        }
    }
    for(i = 0; i < 4; i++) {
        if(! Model.pagecfg.toggle[i])
            continue;
        if (MAINPAGE_GetWidgetLoc(TOGGLE1+i, &x, &y, &w, &h))
            GUI_CreateImageOffset(&gui->toggle[i], x, y, 32, 31, Model.pagecfg.tglico[i]*32, 0, TOGGLE_FILE, NULL, NULL);
    }
    //Battery
    mp->battery = PWR_ReadVoltage();
    if (Display.flags & SHOW_BAT_ICON) {
        GUI_CreateImage(&gui->batt.ico, 270,1,48,22,"media/bat.bmp");
    } else {
        GUI_CreateLabelBox(&gui->batt.lbl, 275,10, 0, 0,
                        mp->battery < Transmitter.batt_alarm ? &BATTALARM_FONT : &BATTERY_FONT,
                        voltage_cb, NULL, NULL);
    }
    //TxPower
    GUI_CreateImageOffset(&gui->pwr, 225,4, 48, 24, 48 * Model.tx_power, 0, "media/txpower.bmp", NULL, NULL);
}

void PAGE_MainExit()
{
    BUTTON_UnregisterCallback(&mp->action);
}

static void _check_voltage()
{
    s16 batt = PWR_ReadVoltage();
    if (batt / 10 != mp->battery / 10 && batt / 10 != mp->battery / 10 + 1) {
        
        mp->battery = batt;
        if(Display.flags & SHOW_BAT_ICON) {
            //FIXME
        } else {
            GUI_SetLabelDesc(&gui->batt.lbl, batt < Transmitter.batt_alarm ? &BATTALARM_FONT : &BATTERY_FONT);
        }
        GUI_Redraw(&gui->batt);
    }
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
            pagemem.modal_page = 1;
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
    if (idx <= NUM_TIMERS) {
        if(press_type == -1 && ! mp->ignore_release) 
            TIMER_StartStop(idx-1);
        mp->ignore_release = 0;
        if(press_type > 0) {
            TIMER_Reset(idx-1);
            mp->ignore_release = 1;
        }
    } else if (idx - NUM_TIMERS <= NUM_TELEM) {
        if(press_type == -1) {
            pagemem.modal_page = 2;
            PAGE_MainExit();
            PAGE_TelemtestModal(PAGE_MainInit, 0);
        }
    }
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    if(! GUI_GetSelected()) {
        if ((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            mp->ignore_release = 1;
            GUI_SetSelected((guiObject_t *)&gui->optico);
        }else if ((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            mp->ignore_release = 1;
            TIMER_Reset(0);
            TIMER_Reset(1);
        } else if (! PAGE_QuickPage(button, flags, data)) {
            MIXER_UpdateTrim(button, flags, data);
        }
        return 1;
    } else {
        if(mp->ignore_release) {
            if (flags & BUTTON_RELEASE)
                mp->ignore_release = 0;
            return 1;
        }
        return 0;
    }
}
