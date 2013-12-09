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
#include "config/ini.h"

#include "../common/_dialogs.c"

static struct dialog_obj * const gui = &gui_objs.dialog;

#define DLG_XOFFSET ((LCD_WIDTH - 320) / 2)
#define DLG_YOFFSET ((LCD_HEIGHT - 240) / 2)

u32 dialogcrc;
void PAGE_ShowSafetyDialog()
{
    if (disable_safety) {
        return; // don't show safety dialog when calibrating
    }
    if (dialog) {
        u64 unsafe = PROTOCOL_CheckSafe();
        if (! unsafe) {
            GUI_RemoveObj(dialog);
            dialog = NULL;
            PROTOCOL_Init(0);
        } else {
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
                snprintf(tempstring + len, sizeof(tempstring) - len, _tr(" is %d%%, safe value = %d%%\n"),
                        val, safeval[Model.safety[i]]);
                if (++count >= 5)
                    break;
            }
            u32 crc = Crc(tempstring, strlen(tempstring));
            if (crc != dialogcrc) {
                GUI_Redraw(dialog);
                dialogcrc = crc;
            }
        }
    } else {
        tempstring[0] = 0;
        dialogcrc = 0;
        dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, _tr("Safety"), NULL, safety_ok_cb, dtOk, tempstring);
    }
}

/******************/
/* Binding Dialog */
/******************/
static void binding_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    dialog = NULL;
}

void PAGE_CloseBindingDialog()
{
    if (dialog) {
        GUI_RemoveObj(dialog);
        dialog = 0;
    }
}

void PAGE_ShowBindingDialog(u8 update)
{
    if (update && ! dialog)
        return;
    u32 bind_time = PROTOCOL_Binding();
    strncpy(tempstring, _tr("Binding is in progress...\nMake sure model is on!\n\nPressing OK will NOT cancel binding procedure\nbut will allow full control of Tx."), sizeof(tempstring));
    u32 len = strlen(tempstring);
    if (bind_time != 0xFFFFFFFF && len < sizeof(tempstring)) {
        snprintf(tempstring + len, sizeof(tempstring) - len, _tr("\n\nBinding will end in %d seconds..."), (int)bind_time / 1000);
    }
    u32 crc = Crc(tempstring, strlen(tempstring));
    if (dialog && crc != dialogcrc) {
        GUI_Redraw(dialog);
    } else if(! dialog) {
        dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, _tr("Binding"), NULL, binding_ok_cb, dtOk, tempstring);
    }
    dialogcrc = crc;
}

void PAGE_ShowWarning(const char *title, const char *str)
{   
    if (dialog)
        return;
    sprintf(tempstring, "%s", str);
    dialogcrc = 0;
    current_selected_obj = GUI_GetSelected();
    dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, title, NULL, lowbatt_ok_cb, dtOk, tempstring);
}

void PAGE_ShowLowBattDialog()
{
    PAGE_ShowWarning(_tr("Low Battery"),
         _tr("Critical battery level detected.\n"
             "Settings have been saved.\n"
             "Any future configuration settings\n"
             "will NOT be saved.\n\n"
             "Change batteries now!"));
}

void PAGE_ShowInvalidStandardMixerDialog(void *guiObj)
{
    (void)guiObj;
    if (dialog)
        return;
    strncpy(tempstring, _tr("Model needs to be reset\nin order to switch to the standard mixer"), sizeof(tempstring));
    tempstring[sizeof(tempstring) - 1] = 0;
    dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, _tr("Standard Mixer"), NULL,
            invalid_stdmixer_cb, dtOkCancel, tempstring);
}

/********************************/
/* Reset Permanent Timer Dialog */
/********************************/

void PAGE_ShowResetPermTimerDialog(void *guiObject, void *data)
{
    (void)guiObject;
    if (dialog)
        return;
    dialogcrc = 0;
    dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, _tr("Reset Permanent Timer?"), reset_timer_string_cb, reset_permtimer_cb, dtOkCancel, data);
}


