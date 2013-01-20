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
#include "gui/gui.h"
#include "pages.h"
#include "config/model.h"
#include "config/ini.h"

#include "../common/_dialogs.c"

#define gui (&gui_objs.dialog)

void PAGE_ShowSafetyDialog()
{
    if (disable_safety) {
        return; // don't show safety dialog when calibrating
    }
    if (dialog == NULL) {
        dlgstr[0] = 0;
        current_selected_obj = GUI_GetSelected();
        dialog = GUI_CreateDialog(&gui->dialog, 2, 5, LCD_WIDTH - 4, LCD_HEIGHT - 10, NULL, NULL, safety_ok_cb, dtOk, dlgstr);
        return;
    }
    u64 unsafe = PROTOCOL_CheckSafe();
    if (! unsafe) {
        GUI_RemoveObj(dialog);
        dialog = NULL;
        PROTOCOL_Init(0);
        return;
    }
    int i;
    int count = 0;
    char tmpstr[30];
    const s8 safeval[4] = {0, -100, 0, 100};
    volatile s16 *raw = MIXER_GetInputs();
    u32 crc = Crc(dlgstr, strlen(dlgstr));
    dlgstr[0] = 0;
    for(i = 0; i < NUM_SOURCES + 1; i++) {
        if (! (unsafe & (1LL << i)))
            continue;
        int ch = (i == 0) ? PROTOCOL_MapChannel(INP_THROTTLE, NUM_INPUTS + 2) : i-1;

        s16 val = RANGE_TO_PCT((ch < NUM_INPUTS)
                      ? raw[ch+1]
                      : MIXER_GetChannel(ch - (NUM_INPUTS), APPLY_SAFETY));
        sprintf(dlgstr + strlen(dlgstr), _tr("%s is %d%%,\nsafe value = %d%%"),
                INPUT_SourceName(tmpstr, ch + 1),
                val, safeval[Model.safety[i]]);
        if (++count >= 5)
            break;
    }
    if (crc != Crc(dlgstr, strlen(dlgstr))) {
        GUI_Redraw(dialog);
    }
}

/******************/
/* Binding Dialog */
/******************/
static void binding_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    PROTOCOL_SetBindState(0); // interrupt the binding
    PAGE_CloseBindingDialog();
}

void PAGE_CloseBindingDialog()
{
    if (dialog) {
        GUI_RemoveObj(dialog);
        GUI_SetSelected(current_selected_obj);
        dialog = NULL;
    }
}

void PAGE_ShowBindingDialog(u8 update)
{
    if (update && ! dialog)
        return;
    u32 crc = Crc(dlgstr, strlen(dlgstr));
    u32 bind_time = PROTOCOL_Binding();
    if (bind_time != 0xFFFFFFFF )
        sprintf(dlgstr, _tr("Binding...\n%d seconds\nPress ENT to stop"), (int)bind_time / 1000);
    u32 crc_new = Crc(dlgstr, strlen(dlgstr));
    if (dialog && crc != crc_new) {
        GUI_Redraw(dialog);
    } else if(! dialog) {
        current_selected_obj = GUI_GetSelected();
        dialog = GUI_CreateDialog(&gui->dialog, 2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, NULL, NULL, binding_ok_cb, dtOk, dlgstr);
    }
}

void PAGE_ShowLowBattDialog()
{
    if (dialog)
        return;
    strncpy(dlgstr, _tr("Battery too low,\ncan't save!"), sizeof(dlgstr));
    dlgstr[sizeof(dlgstr) - 1] = 0;
    current_selected_obj = GUI_GetSelected();
    dialog = GUI_CreateDialog(&gui->dialog, 5, 5, LCD_WIDTH - 10, LCD_HEIGHT - 10, NULL, NULL, lowbatt_ok_cb, dtOk, dlgstr);
}

const char *string_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    strncpy(dlgstr, _tr("Model needs reset\nfor simple mixer"), sizeof(dlgstr));
    return dlgstr;
}

void PAGE_ShowInvalidSimpleMixerDialog(void *guiObj)
{
    if (dialog)
        return;

    dlgstr[sizeof(dlgstr) - 1] = 0;
    current_selected_obj = GUI_GetSelected();
    dialog = GUI_CreateDialog(&gui->dialog, 2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, NULL, string_cb,
            invalid_simplemixer_cb, dtOkCancel, guiObj);
}

void PAGE_ShowInvalidModule()
{
    if (dialog)
        return;
    strncpy(dlgstr, _tr("Bad/missing\nprotocol modules!"), sizeof(dlgstr));
    dlgstr[sizeof(dlgstr) - 1] = 0;
    current_selected_obj = GUI_GetSelected();
    dialog = GUI_CreateDialog(&gui->dialog, 5, 5, LCD_WIDTH - 10, LCD_HEIGHT - 10, NULL, NULL, lowbatt_ok_cb, dtOk, dlgstr);
}
