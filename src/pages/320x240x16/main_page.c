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

#define VTRIM_W      10
#define VTRIM_H     140
#define HTRIM_W     125
#define HTRIM_H      10
#define MODEL_ICO_W  96
#define MODEL_ICO_H  96
#define GRAPH_H      59
#define GRAPH_W      10
#define BOX_W       113
#define SMALLBOX_H   24
#define BIGBOX_H     40

#include "../common/_main_page.c"

void GetElementSize(unsigned type, u16 *w, u16 *h)
{
    const u8 width[ELEM_LAST] = {
        [ELEM_SMALLBOX] = BOX_W,
        [ELEM_BIGBOX]   = BOX_W,
        [ELEM_TOGGLE]   = TOGGLEICON_WIDTH,
        [ELEM_BAR]      = GRAPH_W,
        [ELEM_VTRIM]    = VTRIM_W,
        [ELEM_HTRIM]    = HTRIM_W,
        [ELEM_MODELICO] = MODEL_ICO_W,
    };
    const u8 height[ELEM_LAST] = {
        [ELEM_SMALLBOX] = SMALLBOX_H,
        [ELEM_BIGBOX]   = BIGBOX_H,
        [ELEM_TOGGLE]   = TOGGLEICON_HEIGHT,
        [ELEM_BAR]      = GRAPH_H,
        [ELEM_VTRIM]    = VTRIM_H,
        [ELEM_HTRIM]    = HTRIM_H,
        [ELEM_MODELICO] = MODEL_ICO_H,
    };
    *w = width[type];
    *h = height[type];
}
int GetWidgetLoc(struct elem *elem, u16 *x, u16 *y, u16 *w, u16 *h)
{
    *y = ELEM_Y(*elem);
    if (*y == 0)
        return 0;
    int type = ELEM_TYPE(*elem);
    if (type >= ELEM_LAST)
        return 0;
    *x = ELEM_X(*elem);
    GetElementSize(type, w, h);
    return 1;
}

unsigned map_type(int type)
{
    switch(type) {
        case ELEM_BIGBOX: return ELEM_SMALLBOX;
        case ELEM_HTRIM: return ELEM_VTRIM;
        default: return type;
    }
}
int MAINPAGE_FindNextElem(unsigned type, int idx)
{
    type = map_type(type);
    for(int i = idx; i < NUM_ELEMS; i++) {
        if(! ELEM_USED(pc.elem[i]))
            break;
        if (map_type(ELEM_TYPE(pc.elem[i])) == type)
            return i;
    }
    return -1;
}

void PAGE_MainInit(int page)
{
    (void)page;
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
    if (! MAINPAGE_FindNextElem(ELEM_MODELICO, 0) < 0)
        GUI_CreateIcon(&gui->modelico, 32, 0, &icons[ICON_MODELICO], press_icon2_cb, (void *)1);

    GUI_CreateLabelBox(&gui->name, 96, 8, 128, 24, &MODELNAME_FONT,
                                      NULL, press_icon_cb, Model.name);

    for (int i = 0; i < NUM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.elem[i], &x, &y, &w, &h))
            break;
        int type = ELEM_TYPE(pc.elem[i]);
        switch(type) {
            case ELEM_MODELICO:
                GUI_CreateImageOffset(&gui->elem[i].img, x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), press_icon_cb, (void *)1);
                break;
            case ELEM_VTRIM:
            case ELEM_HTRIM:
            {
                int src = pc.elem[i].src;
                mp->elem[i] = *(MIXER_GetTrim(src));
                GUI_CreateBarGraph(&gui->elem[i].bar, x, y, w, h, -100, 100,
                    type == ELEM_VTRIM ? TRIM_VERTICAL : TRIM_INVHORIZONTAL, trim_cb, (void *)(long)src);
                break;
            }
            case ELEM_SMALLBOX:
            case ELEM_BIGBOX:
            {
                int src = pc.elem[i].src;
                if (src == 0)
                    continue;
                mp->elem[i] = get_boxval(src);
                int font = ((src <= NUM_TIMERS && mp->elem[i] < 0)
                           || ((u8)(src - NUM_TIMERS - 1) < NUM_TELEM && Telemetry.time[0] == 0));
                GUI_CreateLabelBox(&gui->elem[i].box, x, y, w, h,
                            get_box_font(type == ELEM_BIGBOX ? 0 : 2, font),
                            show_box_cb, press_box_cb,
                            (void *)((long)src));
                break;
            }
            case ELEM_BAR:
            {
                int src = pc.elem[i].src;
                if (src == 0)
                    continue;
                mp->elem[i] = MIXER_GetChannel(src-1, APPLY_SAFETY);
                GUI_CreateBarGraph(&gui->elem[i].bar, x, y, w, h, CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                           bar_cb, (void *)((long)src));
                break;
            }
            case ELEM_TOGGLE:
            {
                struct ImageMap img = TGLICO_GetImage(ELEM_ICO(pc.elem[i], 0)); //We'll set this properly down below
                GUI_CreateImageOffset(&gui->elem[i].img, x, y, w, h,
                                  img.x_off, img.y_off, img.file, NULL, NULL);
                break;
            }
        }
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
