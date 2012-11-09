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
#define MIN_BATTERY_ALARM 3300
#define MIN_BATTERY_ALARM_STEP 50

void PAGE_ChangeByName(const char *pageName, u8 menuPage)
{   // dummy method for devo8, only used in devo10
    (void)pageName;
    (void)menuPage;
}
#include "../common/_tx_configure.c"


void PAGE_TxConfigureInit(int page)
{
    (void)page;
    cp->enable = CALIB_NONE;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("Configure"));

    u8 space = 22;
    u8 row = 40;
    GUI_CreateLabelBox(16, row+6, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Language:"));
    GUI_CreateButton(112, row, BUTTON_96, langstr_cb, 0x0000, lang_select_cb, NULL);
    row += space + 8;
    GUI_CreateLabelBox(16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Stick Mode:"));
    GUI_CreateTextSelect(112, row, TEXTSELECT_96, 0x0000, NULL, modeselect_cb, NULL);
    row += space;
    GUI_CreateLabelBox(16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Backlight:"));
    GUI_CreateTextSelect(112, row, TEXTSELECT_96, 0x0000, NULL, brightness_select_cb, NULL);
    row += space;
    GUI_CreateLabelBox(16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer Time:"));
    GUI_CreateTextSelect(112, row, TEXTSELECT_96, 0x0000, NULL, auto_dimmer_time_cb, NULL);
    row += space;
    GUI_CreateLabelBox(16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer Target:"));
    cp->dimmer_target = GUI_CreateTextSelect(112, row, TEXTSELECT_96, 0x0000, NULL, auto_dimmer_value_cb, NULL);
    row += space;
    GUI_CreateLabelBox(16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Battery Alarm:"));
    GUI_CreateTextSelect(112, row, TEXTSELECT_96, 0x0000, NULL, batalarm_select_cb, NULL);
    row += space;
    GUI_CreateLabelBox(16, row+6, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Touch Screen:"));
    GUI_CreateButton(112, row, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH);
    GUI_CreateButton(216, row, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH_TEST);
    row += space +8 ;
    GUI_CreateLabelBox(16, row+6, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Sticks:"));
    GUI_CreateButton(112, row, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK);
    // GUI_CreateButton(216, 140, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK_TEST);
}
