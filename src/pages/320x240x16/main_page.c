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
#include "telemetry.h"
#include "rtc.h"

enum {
    VTRIM_W      = 10,
    VTRIM_H     = 140,
    HTRIM_W     = 125,
    HTRIM_H      = 10,
    MODEL_ICO_W  = 96,
    MODEL_ICO_H  = 96,
    GRAPH_H      = 59,
    GRAPH_W      = 10,
    BOX_W       = 113,
    SMALLBOX_H   = 24,
    BIGBOX_H     = 39,
    BATTERY_W    = 0,
    BATTERY_H    = 0,
    TXPOWER_W    = 0,
    TXPOWER_H    = 0,
};

void press_icon_cb(guiObject_t *obj, s8 press_type, const void *data);
void press_box_cb(guiObject_t *obj, s8 press_type, const void *data);

#include "../common/_main_page.c"

void PAGE_MainInit(int page)
{
    (void)page;
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

    if (HAS_TOUCH) {
        GUI_CreateIcon(&gui->optico, 0, 0, &icons[ICON_OPTIONS], press_icon2_cb, (void *)0);
        GUI_ChangeSelectionOnTouch(0);
    }
    if (MAINPAGE_FindNextElem(ELEM_MODELICO, 0) < 0) {
        void (*cb)(guiObject_t *, const void *) = HAS_TOUCH ? press_icon2_cb : NULL;
        GUI_CreateIcon(&gui->modelico, 32, 0, &icons[ICON_MODELICO], cb, (void *)1);
    }
    GUI_CreateLabelBox(&gui->name, (LCD_WIDTH-128)/2, 8, 128, 24, &MODELNAME_FONT,
                                      NULL, press_icon_cb, Model.name);

    show_elements();
    int left_offset = 45;
#if HAS_RTC
    if(Display.flags & SHOW_TIME) {
        GUI_CreateLabelBox(&gui->time, LCD_WIDTH-35, 10, 34, 20, &BATTERY_FONT, time_cb, NULL, NULL);
        left_offset += 35;
    }
#endif
    //Battery
    mp->battery = PWR_ReadVoltage();
    if (Display.flags & SHOW_BAT_ICON) {
        GUI_CreateImage(&gui->batt.ico, LCD_WIDTH - left_offset - 5,1,48,22,"media/bat" IMG_EXT);
    } else {
        GUI_CreateLabelBox(&gui->batt.lbl, LCD_WIDTH - left_offset,10, 45, 20,
                        mp->battery < Transmitter.batt_alarm ? &BATTALARM_FONT : &BATTERY_FONT,
                        voltage_cb, NULL, NULL);
    }
    //TxPower
    GUI_CreateImageOffset(&gui->pwr, LCD_WIDTH - left_offset - 50,4, 48, 24, 48 * Model.tx_power, 0, "media/txpower" IMG_EXT, NULL, NULL);
}

void PAGE_MainExit()
{
    BUTTON_UnregisterCallback(&mp->action);
}

static void _check_voltage(guiLabel_t *obj)
{
    (void)obj;
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
            PAGE_PushByID(PAGEID_MENU, 0);
        } else if ((long)data == 1) {
            if(HAS_STANDARD_GUI && Model.mixer_mode == MIXER_STANDARD)
                PAGE_PushByID(PAGEID_MODELMNU, 0);
            else
                PAGE_PushByID(PAGEID_MIXER, 0);
        } else {
            PAGE_PushByID(PAGEID_LOADSAVE, LOAD_MODEL);
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
#if HAS_RTC
    if(idx <= NUM_RTC)
        return;
#endif
    if (idx - NUM_RTC <= NUM_TIMERS) {
        if(press_type == -1 && ! mp->ignore_release) 
            TIMER_StartStop(idx - NUM_RTC - 1);
        mp->ignore_release = 0;
        if(press_type > 0) {
            TIMER_Reset(idx - NUM_RTC - 1);
            mp->ignore_release = 1;
        }
    } else if (idx - NUM_RTC - NUM_TIMERS<= NUM_TELEM) {
        if(press_type == -1) {
            pagemem.modal_page = 2;
            PAGE_PushByID(PAGEID_TELEMMON, 0);
        }
    }
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    if(! GUI_GetSelected()) {
        if (flags & BUTTON_RELEASE) {
            if ((flags & BUTTON_HAD_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_ENTER)) {
                //GUI_SetSelected((guiObject_t *)&gui->name);
            } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
                //see pagelist.h for mapping of 'page' to menu_id 
                PAGE_PushByID(PAGEID_MENU, 0);
            } else if ((flags & BUTTON_HAD_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
                for (u8 timer=0; timer<NUM_TIMERS; timer++) {
                    TIMER_Reset(timer);
                }
            } else if (! PAGE_QuickPage(button, flags, data)) {
                MIXER_UpdateTrim(button, flags, data);
            }
        }
        return 1;
    } else {
        return 0;
    }
}
