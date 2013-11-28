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

static u32 dialogcrc;
void PAGE_ShowSafetyDialog()
{
    if (disable_safety) {
        return; // don't show safety dialog when calibrating
    }
    if (dialog == NULL) {
        tempstring[0] = 0;
        dialogcrc = 0;
        current_selected_obj = GUI_GetSelected();
        dialog = GUI_CreateDialog(&gui->dialog, 2, 5, LCD_WIDTH - 4, LCD_HEIGHT - 10, NULL, NULL, safety_ok_cb, dtOk, tempstring);
        return;
    }
    u64 unsafe = PROTOCOL_CheckSafe();
    if (! unsafe) {
        GUI_RemoveObj(dialog);
        GUI_SetSelected(current_selected_obj);
        dialog = NULL;
        PROTOCOL_Init(0);
        return;
    }
    int i;
    int count = 0;
    const s8 safeval[4] = {0, -100, 0, 100};
    volatile s16 *raw = MIXER_GetInputs();
    tempstring[0] = 0;
    for(i = 0; i < NUM_SOURCES + 1; i++) {
        if (! (unsafe & (1LL << i)))
            continue;
        int ch = (i == 0) ? PROTOCOL_MapChannel(INP_THROTTLE, NUM_INPUTS + 2) : i-1;

        s16 val = RANGE_TO_PCT((ch < NUM_INPUTS)
                      ? raw[ch+1]
                      : MIXER_GetChannel(ch - (NUM_INPUTS), APPLY_SAFETY));
        INPUT_SourceName(tempstring + strlen(tempstring), ch + 1);
        int len = strlen(tempstring);
        snprintf(tempstring + len, sizeof(tempstring) - len, _tr(" is %d%%,\nsafe value = %d%%"),
                val, safeval[Model.safety[i]]);
        if (++count >= 5)
            break;
    }
    u32 crc = Crc(tempstring, strlen(tempstring));
    if (crc != dialogcrc) {
        dialogcrc = crc;
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
    u32 bind_time = PROTOCOL_Binding();
    strcpy(tempstring, _tr("Binding...\nPress ENT to stop"));
    if (bind_time != 0xFFFFFFFF ) {
        int len = strlen(tempstring);
        snprintf(tempstring + len, sizeof(tempstring) - len, _tr("\n%d seconds left"), (int)bind_time / 1000);
    }
    u32 crc = Crc(tempstring, strlen(tempstring));
    if (dialog && crc != dialogcrc) {
        GUI_Redraw(dialog);
    } else if(! dialog) {
        current_selected_obj = GUI_GetSelected();
        dialog = GUI_CreateDialog(&gui->dialog, 2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, NULL, NULL, binding_ok_cb, dtOk, tempstring);
    }
    dialogcrc = crc;
}

void PAGE_ShowWarning(const char *title, const char *str)
{
    (void)title;
    if (dialog)
        return;
    strncpy(tempstring, str, sizeof(tempstring));
    tempstring[sizeof(tempstring) - 1] = 0;
    current_selected_obj = GUI_GetSelected();
    dialog = GUI_CreateDialog(&gui->dialog, 5, 5, LCD_WIDTH - 10, LCD_HEIGHT - 10, NULL, NULL, lowbatt_ok_cb, dtOk, tempstring);
}


void PAGE_ShowLowBattDialog()
{
    PAGE_ShowWarning(NULL, _tr("Battery too low,\ncan't save!"));
}

const char *string_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    strncpy(tempstring, _tr("Model needs reset\nfor standard mixer"), sizeof(tempstring));
    return tempstring;
}

void PAGE_ShowInvalidStandardMixerDialog(void *guiObj)
{
    if (dialog)
        return;

    tempstring[sizeof(tempstring) - 1] = 0;
    current_selected_obj = GUI_GetSelected();
    dialog = GUI_CreateDialog(&gui->dialog, 2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, NULL, string_cb,
            invalid_stdmixer_cb, dtOkCancel, guiObj);
}

void PAGE_ShowInvalidModule()
{
    PAGE_ShowWarning(NULL, _tr("Bad/missing\nprotocol modules!"));
}

/********************************/
/* Reset Permanent Timer Dialog */
/********************************/

void PAGE_ShowResetPermTimerDialog(void *guiObject, void *data)
{
    (void)guiObject;
    current_selected_obj = GUI_GetSelected();
    if (dialog)
        return;
    dialog = GUI_CreateDialog(&gui->dialog, 2 , 2,  LCD_WIDTH - 4, LCD_HEIGHT - 4 , NULL , reset_timer_string_cb, reset_permtimer_cb, dtOkCancel, data);
}
