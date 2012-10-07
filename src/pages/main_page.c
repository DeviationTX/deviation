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

static struct main_page * const mp = &pagemem.u.main_page;
const char *show_box_cb(guiObject_t *obj, const void *data);
const char *voltage_cb(guiObject_t *obj, const void *data);
static s16 trim_cb(void * data);
static s16 bar_cb(void * data);
void press_icon_cb(guiObject_t *obj, s8 press_type, const void *data);
void press_icon2_cb(guiObject_t *obj, const void *data);
void press_box_cb(guiObject_t *obj, s8 press_type, const void *data);
static u8 action_cb(u32 button, u8 flags, void *data);

static s32 get_boxval(u8 idx);

struct LabelDesc *get_box_font(u8 idx, u8 neg)
{
    if(neg) {
        return idx & 0x02 ? &SMALLBOXNEG_FONT : &BIGBOXNEG_FONT;
    } else {
        return idx & 0x02 ? &SMALLBOX_FONT : &BIGBOX_FONT;
    }
}

void PAGE_MainInit(int page)
{
    (void)page;
    int i;
    u16 x, y, w, h;
    PAGE_SetModal(0);
    BUTTON_RegisterCallback(&mp->action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);

    mp->optsObj = GUI_CreateIcon(0, 0, &icons[ICON_OPTIONS], press_icon2_cb, (void *)0);
    if(! MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
        GUI_CreateIcon(32, 0, &icons[ICON_MODELICO], press_icon2_cb, (void *)1);

    mp->nameObj = GUI_CreateLabelBox(96, 8, 128, 24, &MODELNAME_FONT,
                                      NULL, press_icon_cb, Model.name);

    //Icon
    if (MAINPAGE_GetWidgetLoc(MODEL_ICO, &x, &y, &w, &h))
        GUI_CreateImageOffset(x, y, w, h, 0, 0, CONFIG_GetCurrentIcon(), press_icon_cb, (void *)1);

    for(i = 0; i < 6; i++) {
        mp->trims[i] = Model.trims[i].value;
        if (MAINPAGE_GetWidgetLoc(TRIM1+i, &x, &y, &w, &h))
            mp->trimObj[i] = GUI_CreateBarGraph(x, y, w, h, -100, 100, i & 0x02 ? TRIM_INVHORIZONTAL : TRIM_VERTICAL, trim_cb, &Model.trims[i].value);
        else
            mp->trimObj[i] = NULL;
    }
    for(i = 0; i < 8; i++) {
        if (MAINPAGE_GetWidgetLoc(BOX1+i, &x, &y, &w, &h)) {
            mp->boxval[i] = get_boxval(Model.pagecfg.box[i]);
            mp->boxObj[i] = GUI_CreateLabelBox(x, y, w, h,
                                get_box_font(i, Model.pagecfg.box[i] <= 2 && mp->boxval[i] < 0),
                                show_box_cb, press_box_cb,
                                (void *)((long)Model.pagecfg.box[i]));
        } else {
            mp->boxval[i] = 0;
            mp->boxObj[i] = NULL;
        }
    }
    for(i = 0; i < 8; i++) {
        if (i >= NUM_OUT_CHANNELS || i >= Model.num_channels)
            break;
        if (MAINPAGE_GetWidgetLoc(BAR1+i, &x, &y, &w, &h)) {
            mp->barval[i] = MIXER_GetChannel(Model.pagecfg.bar[i]-1, APPLY_SAFETY);
            mp->barObj[i] = GUI_CreateBarGraph(x, y, w, h, CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                                               bar_cb, (void *)((long)Model.pagecfg.bar[i]));
        } else {
            mp->barval[i] = 0;
            mp->barObj[i] = NULL;
        }
    }
    for(i = 0; i < 4; i++) {
        if(! Model.pagecfg.toggle[i])
            continue;
        if (MAINPAGE_GetWidgetLoc(TOGGLE1+i, &x, &y, &w, &h))
            mp->toggleObj[i] = GUI_CreateImageOffset(x, y, 32, 31, Model.pagecfg.tglico[i]*32, 0, TOGGLE_FILE, NULL, NULL);
    }
    //Battery
    mp->battery = PWR_ReadVoltage();
    if (Display.flags & SHOW_BAT_ICON) {
        mp->battObj = GUI_CreateImage(270,1,48,22,"media/bat.bmp");
    } else {
        mp->battObj = GUI_CreateLabelBox(275,10, 0, 0,
                        mp->battery < Transmitter.batt_alarm ? &BATTALARM_FONT : &BATTERY_FONT,
                        voltage_cb, NULL, NULL);
    }
    //TxPower
    GUI_CreateImageOffset(225,4, 48, 24, 48 * Model.tx_power, 0, "media/txpower.bmp", NULL, NULL);
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
                GUI_SetLabelDesc(mp->boxObj[i], get_box_font(i, val < 0));
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
    s16 *raw = MIXER_GetInputs();
    for(i = 0; i < 8; i++) {
        if (! mp->barObj[i])
            continue;
        s16 chan = MIXER_GetChannel(Model.pagecfg.bar[i]-1, APPLY_SAFETY);
        if (mp->barval[i] != chan) {
            mp->barval[i] = chan;
            GUI_Redraw(mp->barObj[i]);
        }
    }
    for(i = 0; i < 4; i++) {
        if (! mp->toggleObj[i])
            continue;
        u8 src = MIXER_SRC(Model.pagecfg.toggle[i]);
        s16 val = raw[src];
        GUI_SetHidden(mp->toggleObj[i], MIXER_SRC_IS_INV(Model.pagecfg.toggle[i]) ? val > 0 : val < 0);
    }
    s16 batt = PWR_ReadVoltage();
    if (batt / 10 != mp->battery / 10 && batt / 10 != mp->battery / 10 + 1) {
        
        mp->battery = batt;
        if(Display.flags & SHOW_BAT_ICON) {
            //FIXME
        } else {
            GUI_SetLabelDesc(mp->battObj, batt < Transmitter.batt_alarm ? &BATTALARM_FONT : &BATTERY_FONT);
        }
        GUI_Redraw(mp->battObj);
    }
}

s32 get_boxval(u8 idx)
{
    if (idx <= NUM_TIMERS)
        return TIMER_GetValue(idx - 1);
    if(idx - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_GetValue(idx - NUM_TIMERS - 1);
    return RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY));
}

void PAGE_MainExit()
{
    BUTTON_UnregisterCallback(&mp->action);
}

const char *show_box_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    if (idx <= NUM_TIMERS) {
        TIMER_SetString(mp->tmpstr, TIMER_GetValue(idx - 1));
    } else if(idx - NUM_TIMERS <= NUM_TELEM) {
        TELEMETRY_SetString(mp->tmpstr, idx - NUM_TIMERS - 1);
    } else {
        sprintf(mp->tmpstr, "%3d%%", RANGE_TO_PCT(MIXER_GetChannel(idx - (NUM_TIMERS + NUM_TELEM + 1), APPLY_SAFETY)));
    }
    return mp->tmpstr;
}

const char *voltage_cb(guiObject_t *obj, const void *data) {
    (void)obj;
    (void)data;
    sprintf(mp->tmpstr, "%2d.%02dV", mp->battery / 1000, (mp->battery % 1000) / 10);
    return mp->tmpstr;
}

s16 trim_cb(void * data)
{
    s8 *trim = (s8 *)data;
    return *trim;
}

s16 bar_cb(void * data)
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
}

static u8 action_cb(u32 button, u8 flags, void *data)
{
    if(! GUI_GetSelected()) {
        if ((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            mp->ignore_release = 1;
            GUI_SetSelected(mp->optsObj);
        }else if ((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            mp->ignore_release = 1;
            TIMER_Reset(0);
            TIMER_Reset(1);
        } else {
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
