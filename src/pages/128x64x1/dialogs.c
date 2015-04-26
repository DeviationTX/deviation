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

#define MAX_CONCURRENT_SAFETY_MSGS 1

#include "../common/_dialogs.c"

static struct dialog_obj * const gui = &gui_objs.dialog;


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
        dialog = GUI_CreateDialog(&gui->dialog, 2, 5, LCD_WIDTH - 4, LCD_HEIGHT - 10, NULL, safety_string_cb, safety_ok_cb, dtOk, NULL);
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
    char tmp[10];
    snprintf(tmp, 10, "%s", Model.name);
    snprintf(tempstring, sizeof(tempstring), _tr("Binding %s...\nPress ENT to stop"), tmp);
    if (bind_time != 0xFFFFFFFF ) {
        int len = strlen(tempstring);
        snprintf(tempstring + len, sizeof(tempstring) - len, _tr("\n%d seconds left"), (int)bind_time / 1000);
    }
    return tempstring;
}
static void binding_ok_cb(u8 state, void * data)
{
    (void)state;
    (void)data;
    PROTOCOL_SetBindState(0); // interrupt the binding
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
        dialog = GUI_CreateDialog(&gui->dialog, 2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, NULL, NULL, binding_ok_cb, dtOk, tempstring);
    }
    dialogcrc = crc;
}

void PAGE_ShowWarning(const char *title, const char *str)
{
    (void)title;
    if (dialog)
        return;
    if (str != tempstring)
        tempstring_cpy(str);
    dialog = GUI_CreateDialog(&gui->dialog, 5, 5, LCD_WIDTH - 10, LCD_HEIGHT - 10, NULL, NULL, lowbatt_ok_cb, dtOk, tempstring);
}


void PAGE_ShowLowBattDialog()
{
    PAGE_ShowWarning(NULL, _tr("Battery too low,\ncan't save!"));
}

const char *invalidstdmixer_string_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    return _tr("Model needs reset\nfor standard mixer");
}

void PAGE_ShowInvalidStandardMixerDialog(void *guiObj)
{
    if (dialog)
        return;

    tempstring[sizeof(tempstring) - 1] = 0;
    dialog = GUI_CreateDialog(&gui->dialog, 2, 2, LCD_WIDTH - 4, LCD_HEIGHT - 4, NULL,
            invalidstdmixer_string_cb,
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
    if (dialog)
        return;
    dialog = GUI_CreateDialog(&gui->dialog, 2 , 2,  LCD_WIDTH - 4, LCD_HEIGHT - 4 , NULL , reset_timer_string_cb, reset_permtimer_cb, dtOkCancel, data);
}

/********************************/
/* Show Missing Transceiver Modules */
/********************************/
void PAGE_ShowModuleDialog(const char **missing)
{
    if (dialog)
        return;
    dialogcrc = 0;
    int count = 0;
    sprintf(tempstring, "%s", _tr("Missing Modules:\n"));
    if(missing[MULTIMOD]) {
        sprintf(tempstring+strlen(tempstring), "%s", missing[MULTIMOD]);
    } else {
        for(int i = 0; i < MULTIMOD; i++) {
           if(missing[i]) {
               if(! count) {
                   sprintf(tempstring+strlen(tempstring), " %s", missing[i]);
               } else if(count == 1) {
                   sprintf(tempstring+strlen(tempstring), " ...");
               }
               count++;
           }
        }
    } 
    PAGE_ShowWarning(NULL, tempstring);
}
