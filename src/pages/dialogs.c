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

#include "target.h"
#include "misc.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/ini.h"

#define DLG_STR_LEN (40 * 5)
static guiObject_t *dialog = NULL;
static char dlgstr[DLG_STR_LEN];

/******************/
/*  Safety Dialog */
/******************/
static void safety_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    dialog = NULL;
    PROTOCOL_Init(1);
}

void PAGE_ShowSafetyDialog()
{
    if (dialog) {
        u32 unsafe = PROTOCOL_CheckSafe();
        if (! unsafe) {
            GUI_RemoveObj(dialog);
            dialog = NULL;
            PROTOCOL_Init(0);
        } else {
            int i;
            int count = 0;
            char tmpstr[10];
            char tmpdlg[DLG_STR_LEN];
            const s8 safeval[4] = {0, -100, 0, 100};
            s16 *raw = MIXER_GetInputs();
            tmpdlg[0] = 0;
            for(i = 0; i < NUM_INPUTS + NUM_CHANNELS; i++) {
                if (! (unsafe & (1 << i)))
                    continue;
                s16 val = RANGE_TO_PCT((i < NUM_INPUTS)
                              ? raw[i+1]
                              : MIXER_GetChannel(i - (NUM_INPUTS), APPLY_SAFETY));
                sprintf(tmpdlg + strlen(tmpdlg), _tr("%s is %d%%, safe value = %d%%\n"),
                        INPUT_SourceName(tmpstr, i + 1),
                        val, safeval[Model.safety[i]]);
                if (++count >= 5)
                    break;
            }
            if (strcmp(dlgstr, tmpdlg) != 0) {
                memcpy(dlgstr, tmpdlg, sizeof(dlgstr));
                GUI_Redraw(dialog);
            }
        }
    } else {
        dlgstr[0] = 0;
        dialog = GUI_CreateDialog(10, 42, 300, 188, _tr("Safety"), NULL, safety_ok_cb, dtOk, dlgstr);
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
    char tmpstr[40] = "";
    if (update && ! dialog)
        return;
    u32 crc = Crc(dlgstr, strlen(dlgstr));
    u32 bind_time = PROTOCOL_Binding();
    if (bind_time != 0xFFFFFFFF)
        sprintf(tmpstr, _tr("\n\nBinding will end in %d seconds"), (int)bind_time / 1000);
    sprintf(dlgstr, _tr("Binding is in progress\nMake sure model is on\n\nPressing OK will NOT cancel binding procedure\nbut will allow full control of Tx.%s"), tmpstr);
    u32 crc_new = Crc(dlgstr, strlen(dlgstr));
    if (dialog && crc != crc_new) {
        GUI_Redraw(dialog);
    } else if(! dialog) {
        dialog = GUI_CreateDialog(10, 42, 300, 188, _tr("Binding"), NULL, binding_ok_cb, dtOk, dlgstr);
    }
}
