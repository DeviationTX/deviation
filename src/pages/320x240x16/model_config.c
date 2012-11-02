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
#include "config/model.h"

#include "../common/_model_config.c"

static void show_titlerow(const char *header)
{
    GUI_CreateLabel(8, 10, NULL, TITLE_FONT, (void *)header);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
}

void MODELPAGE_Config()
{
    PAGE_SetModal(1);
    show_titlerow(Model.type == 0 ? _tr("Helicopter") : _tr("Airplane"));
    if (Model.type == 0) {
        u8 i = 40;
        GUI_CreateLabel(8, i, NULL, DEFAULT_FONT, _tr("SwashType:"));
        GUI_CreateTextSelect(136, i, TEXTSELECT_96, 0x0000, NULL, swash_val_cb, NULL);
        i+=24;
        GUI_CreateLabel(8, i, NULL, DEFAULT_FONT, _tr("ELE Inv:"));
        GUI_CreateTextSelect(136, i, TEXTSELECT_96, 0x0000, swashinv_press_cb, swashinv_val_cb, (void *)1);
        i+=24;
        GUI_CreateLabel(8, i, NULL, DEFAULT_FONT, _tr("AIL Inv:"));
        GUI_CreateTextSelect(136, i, TEXTSELECT_96, 0x0000, swashinv_press_cb, swashinv_val_cb, (void *)2);
        i+=24;
        GUI_CreateLabel(8, i, NULL, DEFAULT_FONT, _tr("COL Inv:"));
        GUI_CreateTextSelect(136, i, TEXTSELECT_96, 0x0000, swashinv_press_cb, swashinv_val_cb, (void *)4);
    }
}
