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

#define gui (&gui_objs.dialog)

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
                sprintf(dlgstr + strlen(dlgstr), _tr("%s is %d%%, safe value = %d%%\n"),
                        INPUT_SourceName(tmpstr, ch + 1),
                        val, safeval[Model.safety[i]]);
                if (++count >= 5)
                    break;
            }
            if (crc != Crc(dlgstr, strlen(dlgstr))) {
                GUI_Redraw(dialog);
            }
        }
    } else {
        dlgstr[0] = 0;
        dialog = GUI_CreateDialog(&gui->dialog, 10, 42, 300, 188, _tr("Safety"), NULL, safety_ok_cb, dtOk, dlgstr);
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
    u32 crc = Crc(dlgstr, strlen(dlgstr));
    u32 bind_time = PROTOCOL_Binding();
    strncpy(dlgstr, _tr("Binding is in progress...\nMake sure model is on!\n\nPressing OK will NOT cancel binding procedure\nbut will allow full control of Tx."), sizeof(dlgstr));
    u32 len = strlen(dlgstr);
    if (bind_time != 0xFFFFFFFF && len < sizeof(dlgstr))
        sprintf(dlgstr + len, _tr("\n\nBinding will end in %d seconds..."), (int)bind_time / 1000);
    dlgstr[sizeof(dlgstr) - 1] = 0;
    u32 crc_new = Crc(dlgstr, strlen(dlgstr));
    if (dialog && crc != crc_new) {
        GUI_Redraw(dialog);
    } else if(! dialog) {
        dialog = GUI_CreateDialog(&gui->dialog, 10, 42, 300, 188, _tr("Binding"), NULL, binding_ok_cb, dtOk, dlgstr);
    }
}

void PAGE_ShowWarning(const char *title, const char *str)
{   
    if (dialog)
        return;
    sprintf(dlgstr, "%s", str);
    current_selected_obj = GUI_GetSelected();
    dialog = GUI_CreateDialog(&gui->dialog, 10, 42, 300, 188, title, NULL, lowbatt_ok_cb, dtOk, dlgstr);
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

void PAGE_ShowInvalidSimpleMixerDialog(void *guiObj)
{
    (void)guiObj;
    if (dialog)
        return;
    strncpy(dlgstr, _tr("Model needs to be reset\nin order to switch to the standard mixer"), sizeof(dlgstr));
    dlgstr[sizeof(dlgstr) - 1] = 0;
    dialog = GUI_CreateDialog(&gui->dialog, 10, 42, 300, 188, _tr("Standard Mixer"), NULL,
            invalid_simplemixer_cb, dtOkCancel, dlgstr);
}
