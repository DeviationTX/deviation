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
#include "gui/gui.h"
#include "config/tx.h"
#include "config/model.h"
#include "autodimmer.h"

#define MIN_BATTERY_ALARM_STEP 10
#define MIN_BATTERY_WARNING 5000
#define MAX_BATTERY_WARNING 3600000
#include "../common/_tx_configure.c"

#define gui (&gui_objs.u.tx)

static u8 _action_cb(u32 button, u8 flags, void *data);
static const char *_contrast_select_cb(guiObject_t *obj, int dir, void *data);
static const char *_vibration_state_cb(guiObject_t *obj, int dir, void *data);
static u16 current_selected = 0;  // do not put current_selected into pagemem as it shares the same structure with other pages by using union

static int size_cb(int absrow, void *data)
{
    (void)data;
    switch(absrow) {
#ifndef NO_LANGUAGE_SUPPORT
        case ITEM_LANG:
#endif
        case ITEM_BACKLIGHT:
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
    const void *label = NULL;
    const void *title = NULL;
    void *tgl = NULL;
    void *value = NULL;
    void *but_str = NULL;
    #define X 68
    u8 x = X;

    switch(absrow) {
#ifndef NO_LANGUAGE_SUPPORT
        case ITEM_LANG:
            title = _tr_noop("Generic settings");
            label = _tr_noop("Language:");
            but_str = langstr_cb; tgl = lang_select_cb;
            break;
#endif
	case ITEM_MUSIC:
            label = _tr_noop("Shutdown music");
            value = _music_shutdown_cb;
            break;
        case ITEM_MODE:
            label = _tr_noop("Stick mode:");
            value = modeselect_cb;
            break;
        case ITEM_BATT:
            label = _tr_noop("Batt alarm:");
            value = batalarm_select_cb;
            break;
	case ITEM_ALARM_INTV:
            label = _tr_noop("Alarm intvl:");
            value = batalarmwarn_select_cb;
            break;
        case ITEM_STICKS:
            label = _tr_noop("Sticks:");
            but_str = calibratestr_cb; tgl = press_cb; data = (void *)CALIB_STICK;
            break;
        case ITEM_BUZZ:
            label = _tr_noop("Buzz volume:");
            value = common_select_cb; data = &Transmitter.volume;
            break;
        case ITEM_HAPTIC:
            label = _tr_noop("Vibration:");
            value = _vibration_state_cb; data = &Transmitter.vibration_state;
            break;
        case ITEM_BACKLIGHT:
            title = _tr_noop("LCD settings");
            label = _tr_noop("Backlight:");
            value = brightness_select_cb;
            break;
        case ITEM_CONTRAST:
            label = _tr_noop("Contrast:");
            value = _contrast_select_cb;
            break;
        case ITEM_DIMTIME:
            label = _tr_noop("Dimmer time:");
            value = auto_dimmer_time_cb; x = X+10;
            break;
        case ITEM_DIMVAL:
            label = _tr_noop("Dimmer target:");
            value = common_select_cb; data = &Transmitter.auto_dimmer.backlight_dim_value; x= X+10;
            break;
        case ITEM_PREALERT:
            title = _tr_noop("Timer settings");
            label = _tr_noop("Prealert time:");
            value = prealert_time_cb; data = (void *)0L; x= X+10;
            break;
        case ITEM_PREALERT_IVAL:
            label = _tr_noop("Prealert intvl:");
            value = timer_interval_cb; data = &Transmitter.countdown_timer_settings.prealert_interval; x= X+15;
            break;
        case ITEM_TIMEUP:
            label = _tr_noop("Timeup intvl:");
            value = timer_interval_cb; data = &Transmitter.countdown_timer_settings.timeup_interval; x= X+15;
            break;
        case ITEM_TELEMTEMP:
            title = _tr_noop("Telemetry settings");
            label = _tr_noop("Temperature:");
            value = units_cb; data = (void *)1L;
            break;
        case ITEM_TELEMLEN:
            label = _tr_noop("Length:");
            value = units_cb; data = (void *)0L;
            break;
    }
    if (title) {
        enum LabelType oldType = labelDesc.style;
        labelDesc.style = LABEL_UNDERLINE;
        GUI_CreateLabelBox(&gui->title, 0, y,
                0, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr(title));
        labelDesc.style = oldType;
        y += ITEM_HEIGHT + 1;
    }
    GUI_CreateLabelBox(&gui->label[relrow], 0, y,
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr(label));
    if(but_str) {
        GUI_CreateButtonPlateText(&gui->value[relrow].but, x, y,
            LCD_WIDTH - ARROW_WIDTH - x - 1, ITEM_HEIGHT, &DEFAULT_FONT, but_str, 0x0000, tgl, data);
    } else {
        GUI_CreateTextSelectPlate(&gui->value[relrow].ts, x, y,
            LCD_WIDTH - ARROW_WIDTH - x - 1, ITEM_HEIGHT, &DEFAULT_FONT, NULL, value, data);
    }
    return 1;
}

void PAGE_TxConfigureInit(int page)
{
    (void)page;
    cp->enable = CALIB_NONE;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("Configure"));
    cp->total_items = 0;


    GUI_CreateScrollable(&gui->scrollable, 0, ITEM_HEIGHT + 1, LCD_WIDTH, LCD_HEIGHT - ITEM_HEIGHT -1,
                     ITEM_SPACE, ITEM_LAST, row_cb, getobj_cb, size_cb, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
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
    }
    if (Transmitter.contrast == 0)
        return _tr("Off");
    sprintf(cp->tmpstr, "%d", Transmitter.contrast);
    return cp->tmpstr;
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

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static inline guiObject_t *_get_obj(int idx, int objid)
{
    return GUI_GetScrollableObj(&gui->scrollable, idx, objid);
}
