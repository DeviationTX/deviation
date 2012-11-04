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

#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 5500
#define MIN_BATTERY_ALARM_STEP 10
#include "../common/_tx_configure.c"

#define VIEW_ID 0

static u8 _action_cb(u32 button, u8 flags, void *data);
static void _navigate_items(s8 direction);
static const char *_contrast_select_cb(guiObject_t *obj, int dir, void *data);

void PAGE_TxConfigureInit(int page)
{
    if (page < 0 && cp->selected > 0) // enter this page from childen page , so we need to get its previous selected item
        page = cp->selected;
    cp->enable = CALIB_NONE;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("Configure"));
    cp->total_items = 0;
    cp->selected = 0;

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 space = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - view_origin_absoluteY ,
            view_origin_absoluteX, view_origin_absoluteY);

    u8 row = 0;
    u8 w = 55;
    u8 x = 68;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Language:"));
    guiObject_t *obj = GUI_CreateButton(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            BUTTON_DEVO10, langstr_cb, 0x0000, lang_select_cb, NULL);
    GUI_CustomizeButton(obj, &DEFAULT_FONT, w, ITEM_HEIGHT);
    GUI_SetSelected(obj);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Stick Mode:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, modeselect_cb, NULL);
    cp->total_items++;

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
            0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Battery Alarm:"));
    GUI_CreateTextSelectPlate(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                    w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, batalarm_select_cb, NULL);
    cp->total_items++;

    row += space;
    GUI_CreateLabelBox(GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
            0, ITEM_HEIGHT,  &DEFAULT_FONT, NULL, NULL, _tr("Sticks:"));
    obj = GUI_CreateButton(GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
            BUTTON_DEVO10, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK);
    GUI_CustomizeButton(obj, &DEFAULT_FONT, w, ITEM_HEIGHT);
    cp->total_items++;

    // The following items are not draw in the logical view;
    cp->scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - 3, space, LCD_HEIGHT- space, cp->total_items, NULL, NULL, NULL);
    if (page > 0)
        _navigate_items(page);
}

static const char *_contrast_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Transmitter.contrast = GUI_TextSelectHelper(Transmitter.contrast,
                                  MIN_BRIGHTNESS, 9, dir, 1, 1, &changed);
    if (changed) {
        int contrast = 0x20 + Transmitter.contrast * 0xC / 9;
        LCD_set_contrast(contrast);
    }
    if (Transmitter.contrast == 0)
        return _tr("Off");
    sprintf(cp->tmpstr, "%d", Transmitter.contrast);
    return cp->tmpstr;
}

static void _navigate_items(s8 direction)
{
    guiObject_t *obj;
    for (u8 i = 0; i < (direction >0 ?direction:-direction); i++) {
        obj = GUI_GetSelected();
        if (direction > 0) {
            GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
        } else {
            GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
        }
    }
    cp->selected += direction;
    cp->selected %= cp->total_items;
    if (cp->selected == 0) {
        GUI_SetRelativeOrigin(VIEW_ID, 0, 0);
    } else if (cp->selected < 0) {
        cp->selected = cp->total_items - 1;
        u8 pages = cp->total_items /PAGE_ITEM_COUNT;
        GUI_SetRelativeOrigin(VIEW_ID, 0, pages * PAGE_ITEM_COUNT * (ITEM_HEIGHT +1));
    } else {
        obj = GUI_GetSelected();
        if (!GUI_IsObjectInsideCurrentView(VIEW_ID, obj)) {
            GUI_ScrollLogicalView(VIEW_ID, (ITEM_HEIGHT +1) *direction);
        }
    }
    GUI_SetScrollbar(cp->scroll_bar, cp->selected);
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("SubMenu", sub_menu_item);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            _navigate_items(-1);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            _navigate_items(1);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
