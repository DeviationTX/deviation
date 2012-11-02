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
#include "config/tx.h"

#include <stdlib.h>

#define HELI_LABEL "Helicopter"  // string too long for devo10, so define it separately for devo8 and devo10
#define PLANE_LABEL "Airplane"
#include "../common/_model_page.c"

void PAGE_ModelInit(int page)
{
    (void)page;
    u8 row;

    mp->file_state = 0;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Model"));

    row = 40;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("File:"));
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, file_press_cb, file_val_cb, NULL);

    row += 32;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Model Name:"));
    GUI_CreateButton(136, row, BUTTON_96x16, show_text_cb, 0x0000, _changename_cb, Model.name);
    GUI_CreateButton(236, row, BUTTON_64x16, show_text_cb, 0x0000, changeicon_cb, _tr("Icon"));

    row += 20;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Model Type:"));
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, type_press_cb, type_val_cb, NULL);

    row += 32;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Protocol:"));
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, protoselect_cb, NULL);

    row += 20;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Number of Channels:"));
    mp->chanObj = GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, numchanselect_cb, NULL);


    row += 32;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Tx Power:"));
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, powerselect_cb, NULL);

    row += 20;
    if(Model.fixed_id == 0)
        strncpy(mp->fixed_id, _tr("None"), sizeof(mp->fixed_id));
    else
        sprintf(mp->fixed_id, "%d", (int)Model.fixed_id);
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Fixed ID:"));
    GUI_CreateButton(136, row, BUTTON_96x16, show_text_cb, 0x0000, fixedid_cb, mp->fixed_id);
    mp->obj = GUI_CreateButton(236, row, BUTTON_64x16, show_bindtext_cb, 0x0000, bind_cb, NULL);
    configure_bind_button();
}

/* Button callbacks */
static void _changename_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    GUI_RemoveObj(obj);
    //Save model info here so it shows up on the model page
    CONFIG_SaveModelIfNeeded();
    PAGE_ModelInit(0);
}

static void _changename_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    PAGE_RemoveAllObjects();
    GUI_CreateKeyboard(KEYBOARD_ALPHA, Model.name, sizeof(Model.name)-1, _changename_done_cb, NULL);
}
