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

#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 5500
#define MIN_BATTERY_ALARM_STEP 10
#include "../common/_tx_configure.c"

#define VIEW_ID 0

static u8 _action_cb(u32 button, u8 flags, void *data);
static const char *_contrast_select_cb(guiObject_t *obj, int dir, void *data);
static const char *_vibration_state_cb(guiObject_t *obj, int dir, void *data);
static s16 view_origin_relativeY;
static s8 current_selected = 0;  // do not put current_selected into pagemem as it shares the same structure with other pages by using union

void PAGE_TxConfigureInit(int page)
{
    if (page < 0 && current_selected > 0) // enter this page from childen page , so we need to get its previous selected item
        page = current_selected;
    cp->enable = CALIB_NONE;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("Configure"));
    cp->total_items = 0;
    current_selected = 0;

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 space = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH - ARROW_WIDTH, LCD_HEIGHT - view_origin_absoluteY ,
            view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    u8 w = 55;
    u8 x = 68;
    enum LabelType oldType = labelDesc.style;
    labelDesc.style = LABEL_UNDERLINE;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Generic settings"));

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Language:"));
    guiObject_t *obj = GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &labelDesc, langstr_cb, 0x0000, lang_select_cb, NULL);
    GUI_SetSelected(obj);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Stick mode:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, modeselect_cb, NULL);
    cp->total_items++;
    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Batt alarm:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                    w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, batalarm_select_cb, NULL);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Sticks:"));
    GUI_CreateButtonPlateText(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &labelDesc, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Buzz volume:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, common_select_cb, (void *)&Transmitter.volume);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Vibration:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, _vibration_state_cb, &Transmitter.vibration_state);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("LCD settings"));
    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Backlight:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, brightness_select_cb, NULL);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Contrast:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, _contrast_select_cb, NULL);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer time:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x +10), GUI_MapToLogicalView(VIEW_ID, row),
                w -10, ITEM_HEIGHT, &DEFAULT_FONT, NULL, auto_dimmer_time_cb, NULL);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer target:"));
    cp->dimmer_target = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x + 10), GUI_MapToLogicalView(VIEW_ID, row),
                w -10, ITEM_HEIGHT, &DEFAULT_FONT, NULL, common_select_cb,
                (void *)&Transmitter.auto_dimmer.backlight_dim_value);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Timer settings"));
    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Prealert time:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x + 10), GUI_MapToLogicalView(VIEW_ID, row),
            w -10, ITEM_HEIGHT, &DEFAULT_FONT, NULL, prealert_time_cb, (void *)0L);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Prealert intvl:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x + 15), GUI_MapToLogicalView(VIEW_ID, row),
            w -15, ITEM_HEIGHT, &DEFAULT_FONT, NULL, timer_interval_cb, &Transmitter.countdown_timer_settings.prealert_interval);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Timeup intvl:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x + 15), GUI_MapToLogicalView(VIEW_ID, row),
            w -15, ITEM_HEIGHT, &DEFAULT_FONT, NULL, timer_interval_cb, &Transmitter.countdown_timer_settings.timeup_interval);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Telemetry settings"));
    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Temperature:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                    w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, units_cb, (void *)1L);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Length:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                    w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, units_cb, (void *)0L);
    cp->total_items++;

    // The following items are not draw in the logical view;
    cp->scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT- ITEM_HEIGHT, cp->total_items, NULL, NULL, NULL);
    if (page > 0)
        PAGE_NavigateItems(page, VIEW_ID, cp->total_items, &current_selected, &view_origin_relativeY, cp->scroll_bar);
    labelDesc.style = oldType;
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
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            PAGE_NavigateItems(-1, VIEW_ID, cp->total_items, &current_selected, &view_origin_relativeY, cp->scroll_bar);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            PAGE_NavigateItems(1, VIEW_ID, cp->total_items, &current_selected, &view_origin_relativeY, cp->scroll_bar);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
