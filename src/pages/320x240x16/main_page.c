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

int GetWidgetLoc(void *ptr, u16 *x, u16 *y, u16 *w, u16 *h)
{
    if ((struct elem_trim *)ptr >= pc.trim && (struct elem_trim *)ptr < pc.trim + NUM_TRIM_ELEMS) {
        //Trim
        struct elem_trim *p = ptr;
        if (p->y == 0)
            return 0;
        *x = p->x;
        *y = p->y;
        int vert = p->is_vert;
        if (vert) {
            *w = VTRIM_W;
            *h = VTRIM_H;
        } else {
            *w = HTRIM_W;
            *h = HTRIM_H;
        }
    } else if ((struct elem_toggle *)ptr >= pc.tgl && (struct elem_toggle *)ptr < pc.tgl + NUM_TOGGLE_ELEMS) {
        //Toggle
        struct elem_toggle *p = ptr;
        if (p->y == 0)
            return 0;
        *x = p->x;
        *y = p->y;
        *w = TOGGLEICON_WIDTH;
        *h = TOGGLEICON_HEIGHT;
    } else if ((struct elem_box *)ptr >= pc.box && (struct elem_box *)ptr < pc.box + NUM_BOX_ELEMS) {
        //Toggle
        struct elem_box *p = ptr;
        if (p->y == 0)
            return 0;
        *x = p->x;
        *y = p->y;
        *w = BOX_W;
        *h = p->type ? BIGBOX_H : SMALLBOX_H;
    } else if (ptr == &pc.modelico) {
        //Model Icon
        struct elem_modelico *p = ptr;
        if (p->y == 0)
            return 0;
        *x = p->x;
        *y = p->y;
        *w = MODEL_ICO_W;
        *h = MODEL_ICO_H;
    } else if ((struct elem_bar *)ptr >= pc.bar && (struct elem_bar *)ptr <= pc.bar + NUM_BAR_ELEMS) {
        //Bar
        struct elem_bar *p = ptr;
        if (p->y == 0)
            return 0;
        *x = p->x;
        *y = p->y;
        *w = GRAPH_W;
        *h = GRAPH_H;
    } else {
        return 0;
    }
    return 1;
}

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
    if(! GetWidgetLoc(&pc.modelico, &x, &y, &w, &h))
        GUI_CreateIcon(&gui->model.ico, 32, 0, &icons[ICON_MODELICO], press_icon2_cb, (void *)1);

    GUI_CreateLabelBox(&gui->name, 96, 8, 128, 24, &MODELNAME_FONT,
                                      NULL, press_icon_cb, Model.name);

    //Icon
    if (GetWidgetLoc(&pc.modelico, &x, &y, &w, &h))
        GUI_CreateImageOffset(&gui->model.img, x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), press_icon_cb, (void *)1);

    for(i = 0; i < NUM_TRIM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.trim[i], &x, &y, &w, &h))
            break;
        int src = pc.trim[i].src;
        int vert = pc.trim[i].is_vert;
        mp->trims[i] = *(MIXER_GetTrim(src));
        GUI_CreateBarGraph(&gui->trim[i], x, y, w, h, -100, 100,
             vert ? TRIM_VERTICAL : TRIM_INVHORIZONTAL, trim_cb, (void *)(long)src);
    }
    for(i = 0; i < NUM_BOX_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.box[i], &x, &y, &w, &h))
            break;
        int src = pc.box[i].src;
        mp->boxval[i] = get_boxval(src);
        int font = ((src <= NUM_TIMERS && mp->boxval[i] < 0) ||
                    ((u8)(src - NUM_TIMERS - 1) < NUM_TELEM && Telemetry.time[0] == 0));
        GUI_CreateLabelBox(&gui->box[i], x, y, w, h,
                            get_box_font(pc.box[i].type ? 0 : 2, font),
                            show_box_cb, press_box_cb,
                            (void *)((long)src));
    }
    for(i = 0; i < NUM_BAR_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.bar[i], &x, &y, &w, &h))
            break;
        int src = pc.bar[i].src;
        mp->barval[i] = MIXER_GetChannel(src-1, APPLY_SAFETY);
        GUI_CreateBarGraph(&gui->bar[i], x, y, w, h, CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                           bar_cb, (void *)((long)src));
    }
    for(i = 0; i < NUM_TOGGLE_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.tgl[i], &x, &y, &w, &h))
            break;
        struct ImageMap img = TGLICO_GetImage(pc.tgl[i].ico[0]); //We'll set this properly down below
        GUI_CreateImageOffset(&gui->toggle[i], x, y, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT,
                                  img.x_off, img.y_off, img.file, NULL, NULL);
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
