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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/tx.h"
#include "config/model.h"
#include "autodimmer.h"
#include "extended_audio.h"

enum {
  LARGE_SEL_X_OFFSET = 68,
  MED_SEL_X_OFFSET   = 68 + 8,
  SMALL_SEL_X_OFFSET = 68 + 15,
  TITLE_X_OFFSET     = 0,
  TITLE_WIDTH        = LCD_WIDTH - ARROW_WIDTH - 1,
  LABEL_X_OFFSET     = 0,
  LABEL_WIDTH        = 0,
  CALIB_Y            = 0,
};
#define TEXTSEL_X_WIDTH (LCD_WIDTH - ARROW_WIDTH - x - 1)
#endif //#ifndef OVERRIDE_PLACEMENT

static const int MIN_BATTERY_ALARM_STEP = 10;
#include "../common/_tx_configure.c"

static struct tx_obj * const gui = &gui_objs.u.tx;

static const char *_contrast_select_cb(guiObject_t *obj, int dir, void *data);
static const char *_vibration_state_cb(guiObject_t *obj, int dir, void *data);
static const char *_buzz_vol_cb(guiObject_t *obj, int dir, void *data);
#if HAS_EXTENDED_AUDIO
static const char *_audio_vol_cb(guiObject_t *obj, int dir, void *data);
#endif

static u16 current_selected = 0;  // do not put current_selected into pagemem as it shares the same structure with other pages by using union

static int size_cb(int absrow, void *data)
{
    (void)data;
    switch(absrow) {
#if SUPPORT_MULTI_LANGUAGE
        case ITEM_LANG:
#else
        case ITEM_MODE:
#endif
        case ITEM_BUZZ:
#ifndef HAS_OLED_DISPLAY
        case ITEM_BACKLIGHT:
#else
        case ITEM_CONTRAST:
#endif
        case ITEM_PREALERT:
        case ITEM_TELEMTEMP:
            return 2;
    }
    return 1;
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)col;
    (void)data;
    return (guiObject_t *)&gui->value[relrow];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    data = NULL;
    const void *label = "";
    const void *title = NULL;
    void *tgl = NULL;
    void *value = NULL;
    void *but_str = NULL;
    u8 x = LARGE_SEL_X_OFFSET;

    switch(absrow) {
#if SUPPORT_MULTI_LANGUAGE
        case ITEM_LANG:
            title = _tr_noop("Generic settings");
            label = _tr_noop("Language");
            but_str = langstr_cb; tgl = lang_select_cb;
            break;
#endif
        case ITEM_MODE:
#if !SUPPORT_MULTI_LANGUAGE
            title = _tr_noop("Generic settings");
#endif
            label = _tr_noop("Stick mode");
            value = modeselect_cb;
            break;
        case ITEM_STICKS:
            label = _tr_noop("Sticks");
            but_str = calibratestr_cb; tgl = press_cb; data = (void *)CALIB_STICK;
            break;
        case ITEM_BUZZ:
#if HAS_EXTENDED_AUDIO
            title = _tr_noop("Audio settings");
#else
            title = _tr_noop("Buzz settings");
#endif
            label = _tr_noop("Buzz volume");
            value = _buzz_vol_cb; data = &Transmitter.volume; x = MED_SEL_X_OFFSET;
            break;
#if HAS_EXTENDED_AUDIO
        case ITEM_AUDIO:
            label = _tr_noop("Audio volume");
            value = _audio_vol_cb; data = &Transmitter.audio_vol; x = MED_SEL_X_OFFSET;
            break;
#endif
        case ITEM_HAPTIC:
            label = _tr_noop("Vibration");
            value = _vibration_state_cb; data = &Transmitter.vibration_state; x = MED_SEL_X_OFFSET;
            break;
        case ITEM_BATT:
            label = _tr_noop("Batt alarm");
            value = batalarm_select_cb; x = MED_SEL_X_OFFSET;
            break;
        case ITEM_ALARM_INTV:
            label = _tr_noop("Alarm intvl");
            value = batalarmwarn_select_cb; x = MED_SEL_X_OFFSET;
            break;
        case ITEM_PWR_ALARM:
            label = _tr_noop("PwrOn alarm");
            value = poweralarm_select_cb; x = MED_SEL_X_OFFSET;
            break;
        case ITEM_PWRDN_ALARM:
            label = _tr_noop("PwrDn alert");
            value = _music_shutdown_cb; x = MED_SEL_X_OFFSET;
            break;
#ifndef HAS_OLED_DISPLAY
        case ITEM_BACKLIGHT:
            title = _tr_noop("LCD settings");
            label = _tr_noop("Backlight");
            value = backlight_select_cb; x = SMALL_SEL_X_OFFSET;
            break;
#endif
        case ITEM_CONTRAST:
#ifdef HAS_OLED_DISPLAY
            title = _tr_noop("LCD settings");
#endif
            label = _tr_noop("Contrast");
            value = _contrast_select_cb; x = SMALL_SEL_X_OFFSET;
            break;
        case ITEM_DIMTIME:
            label = _tr_noop("Dimmer time");
            value = auto_dimmer_time_cb; x = SMALL_SEL_X_OFFSET;
            break;
        case ITEM_DIMVAL:
            label = _tr_noop("Dimmer target");
            value = common_select_cb; data = &Transmitter.auto_dimmer.backlight_dim_value; x = SMALL_SEL_X_OFFSET;
            break;
        case ITEM_PREALERT:
            title = _tr_noop("Timer settings");
            label = _tr_noop("Prealert time");
            value = prealert_time_cb; data = (void *)0L; x = SMALL_SEL_X_OFFSET;
            break;
        case ITEM_PREALERT_IVAL:
            label = _tr_noop("Prealert intvl");
            value = timer_interval_cb; data = &Transmitter.countdown_timer_settings.prealert_interval; x = SMALL_SEL_X_OFFSET;
            break;
        case ITEM_TIMEUP:
            label = _tr_noop("Timeup intvl");
            value = timer_interval_cb; data = &Transmitter.countdown_timer_settings.timeup_interval; x = SMALL_SEL_X_OFFSET;
            break;
        case ITEM_TELEMTEMP:
            title = _tr_noop("Telemetry settings");
            label = _tr_noop("Temperature");
            value = units_cb; data = (void *)1L; x = MED_SEL_X_OFFSET;
            break;
        case ITEM_TELEMLEN:
            label = _tr_noop("Length");
            value = units_cb; data = (void *)0L; x = MED_SEL_X_OFFSET;
            break;
        case ITEM_TELEM_IVAL:
            label = _tr_noop("Alert intvl");
            value = telem_interval_cb; data = &Transmitter.telem_alert_interval; x = MED_SEL_X_OFFSET;
            break;
    }
    if (title) {
        GUI_CreateLabelBox(&gui->title[relrow], TITLE_X_OFFSET, y,
                TITLE_WIDTH, LINE_HEIGHT, &SECTION_FONT, GUI_Localize, NULL, title);
        y += LINE_SPACE;
    }
    GUI_CreateLabelBox(&gui->label[relrow], LABEL_X_OFFSET, y,
            LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, GUI_Localize, NULL, label);
    if(but_str) {
        GUI_CreateButtonPlateText(&gui->value[relrow].but, x, y,
            TEXTSEL_X_WIDTH, LINE_HEIGHT, &BUTTON_FONT, but_str, tgl, data);
    } else {
        GUI_CreateTextSelectPlate(&gui->value[relrow].ts, x, y,
            TEXTSEL_X_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, value, data);
    }
    return 1;
}

void PAGE_TxConfigureInit(int page)
{
    (void)page;
    cp->enable = CALIB_NONE;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr_noop("Configure"));

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, ITEM_LAST, row_cb, getobj_cb, size_cb, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

static const char *_contrast_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Transmitter.contrast = GUI_TextSelectHelper(Transmitter.contrast,
                                  0, 10, dir, 1, 1, &changed);
    if (changed) {
        LCD_Contrast(Transmitter.contrast);
#ifdef HAS_OLED_DISPLAY
        Transmitter.backlight = Transmitter.contrast;
#endif
    }
    if (Transmitter.contrast == 0)
        return _tr("Off");
    sprintf(tempstring, "%d", Transmitter.contrast);
    return tempstring;
}
static const char *_vibration_state_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Transmitter.vibration_state = GUI_TextSelectHelper(Transmitter.vibration_state, 0, 1, dir, 1, 1, NULL);
    if (Transmitter.vibration_state == 0)
        return _tr("Off");
    else
        return _tr("On");
}

static inline guiObject_t *_get_obj(int idx, int objid)
{
    return GUI_GetScrollableObj(&gui->scrollable, idx, objid);
}
