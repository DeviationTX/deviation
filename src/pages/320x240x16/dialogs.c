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
#include "crsf.h"

#define MAX_CONCURRENT_SAFETY_MSGS 5

#include "../common/_dialogs.c"

static struct dialog_obj * const gui = &gui_objs.dialog;

static const int DLG_XOFFSET = ((LCD_WIDTH - 320) / 2);
static const int DLG_YOFFSET = ((LCD_HEIGHT - 240) / 2);

void PAGE_ShowSafetyDialog()
{
    if (dialog) {
        u64 unsafe = safety_check();
        if (! unsafe) {
            DialogClose(dialog, 0);
            safety_confirmed();
        } else {
            safety_string_cb(NULL, NULL);
            u32 crc = Crc(tempstring, strlen(tempstring));
            if (crc != dialogcrc) {
                GUI_Redraw(dialog);
                dialogcrc = crc;
            }
        }
    } else {
        tempstring[0] = 0;
        dialogcrc = 0;
        dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, _tr("Safety"), safety_string_cb, safety_ok_cb, dtOk, NULL);
    }
}

/******************/
/* Binding Dialog */
/******************/
static const char *binding_string_cb(guiObject_t *obj, void *data)
{
    (void)data;
    u32 crc = Crc(tempstring, strlen(tempstring));
    if (obj && crc == dialogcrc)
        return tempstring;
    u32 bind_time = PROTOCOL_Binding();
    tempstring_cpy(_tr("Binding is in progress...\nMake sure model is on!\n\nPressing OK will NOT cancel binding procedure\nbut will allow full control of Tx."));
    u32 len = strlen(tempstring);
    if (bind_time != 0xFFFFFFFF && len < sizeof(tempstring)) {
        snprintf(tempstring + len, sizeof(tempstring) - len, _tr("\n\nBinding will end in %d seconds..."), (int)bind_time / 1000);
    }
    return tempstring;
}
static void binding_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    dialog = NULL;
}

void PAGE_CloseBindingDialog()
{
    if (dialog) {
        DialogClose(dialog, 0);
    }
}

void PAGE_ShowBindingDialog(u8 update)
{
    if (update && ! dialog)
        return;
    binding_string_cb(NULL, NULL);
    u32 crc = Crc(tempstring, strlen(tempstring));
    if (dialog && crc != dialogcrc) {
        GUI_Redraw(dialog);
    } else if(! dialog) {
        const char *title = Model.name[0] ? Model.name : _tr("Binding");
        dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, title, binding_string_cb, binding_ok_cb, dtOk, NULL);
    }
    dialogcrc = crc;
}

void PAGE_ShowWarning(const char *title, const char *str)
{   
    if (dialog)
        return;
    snprintf(dlg_string, sizeof(dlg_string), "%s", str);
    dialogcrc = 0;
    dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188, title, NULL, lowbatt_ok_cb, dtOk, dlg_string);
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

const char *invalidstdmixer_string_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    return _tr("Model needs to be reset\nin order to switch to the standard mixer");
}
void PAGE_ShowInvalidStandardMixerDialog(void *guiObj)
{
    if (dialog)
        return;
    dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188,
            _tr("Standard Mixer"), invalidstdmixer_string_cb, invalid_stdmixer_cb, dtOkCancel,
            guiObj);
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

void PAGE_ShowModuleDialog(const char **missing)
{
    if (dialog)
        return;
    dialogcrc = 0;
    tempstring_cpy(_tr("Missing Modules:\n"));
    for(int i = 0; i < TX_MODULE_LAST; i++) {
       if(missing[i]) {
           sprintf(tempstring+strlen(tempstring), "%s\n", missing[i]);
       }
    } 
    PAGE_ShowWarning(_tr("Module Error"), tempstring);
}

#if SUPPORT_CRSF_CONFIG
/*********************************/
/*   CRSF configuration dialog   */
/*********************************/
void PAGE_CRSFdialog(int status, void *param) {
    if (dialog) {
        GUI_Redraw(dialog);
        return;
    }

    dialog = GUI_CreateDialog(&gui->dialog, 10 + DLG_XOFFSET, 42 + DLG_YOFFSET, 300, 188,
                NULL, cmd_info_cb, crsf_confirm_cb,
                status == CONFIRMATION_NEEDED ? dtOkCancel : dtCancel, param);
}

void PAGE_CRSFdialogClose() {
    if (dialog) {
        DialogClose(dialog, 0);
    }
}
#endif  // SUPPORT_CRSF_CONFIG
