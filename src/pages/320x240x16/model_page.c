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

#define HELI_LABEL _tr_noop("Helicopter")  // string too long for devo10, so define it separately for devo8 and devo10
#define PLANE_LABEL _tr_noop("Airplane")
#include "../common/_model_page.c"

static const char *_mixermode_cb(guiObject_t *obj, int dir, void *data)
{
    const char *ret = mixermode_cb(obj, dir, data);
    if(Model.mixer_mode != mp->last_mixermode) {
        mp->last_mixermode = Model.mixer_mode;
        PAGE_RemoveHeader();
        if (Model.mixer_mode == MIXER_STANDARD)
            PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_MODEL), MODELMENU_Show);
        else
            PAGE_ShowHeader(PAGE_GetName(PAGEID_MODEL));
    }
    return ret;
}

void PAGE_ModelInit(int page)
{
    (void)page;
    u8 row;

    mp->last_mixermode = Model.mixer_mode;
    mp->last_txpower = Model.tx_power;
    mp->file_state = 0;
    PAGE_SetModal(0);
    if (Model.mixer_mode == MIXER_STANDARD)
        PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_MODEL), MODELMENU_Show);
    else
        PAGE_ShowHeader(PAGE_GetName(PAGEID_MODEL));

    row = 40;
    GUI_CreateLabel(&gui->filelbl, 8, row, NULL, DEFAULT_FONT, _tr("File"));
    GUI_CreateTextSelect(&gui->file, 136, row, TEXTSELECT_96, file_press_cb, file_val_cb, NULL);

    row+= 20;
    GUI_CreateLabel(&gui->guilbl, 8, row, NULL, DEFAULT_FONT, _tr("Mixer GUI"));
    GUI_CreateTextSelect(&gui->guits, 136, row, TEXTSELECT_96, NULL, _mixermode_cb, NULL);

    row += 20;
    GUI_CreateLabel(&gui->namelbl, 8, row, NULL, DEFAULT_FONT, _tr("Model name"));  // use the same naming convention for devo8 and devo10
    GUI_CreateButton(&gui->name, 136, row, BUTTON_96x16, show_text_cb, 0x0000, _changename_cb, Model.name);
    GUI_CreateButton(&gui->icon, 236, row, BUTTON_64x16, show_text_cb, 0x0000, changeicon_cb, _tr("Icon"));

    row += 20;
    GUI_CreateLabel(&gui->typelbl, 8, row, NULL, DEFAULT_FONT, _tr("Model type"));
    GUI_CreateTextSelect(&gui->type, 136, row, TEXTSELECT_96, type_press_cb, type_val_cb, NULL);

    row += 24;
    GUI_CreateLabel(&gui->ppmlbl, 8, row, NULL, DEFAULT_FONT, _tr("PPM In"));
    GUI_CreateTextSelect(&gui->ppm, 136, row, TEXTSELECT_96, ppmin_press_cb, ppmin_select_cb, NULL);

    row += 20;
    GUI_CreateLabel(&gui->protolbl, 8, row, NULL, DEFAULT_FONT, _tr("Protocol"));
    GUI_CreateTextSelect(&gui->proto, 136, row, TEXTSELECT_96, proto_press_cb, protoselect_cb, NULL);

    row += 20;
    GUI_CreateLabel(&gui->numchlbl, 8, row, NULL, DEFAULT_FONT, _tr("# Channels"));
    GUI_CreateTextSelect(&gui->numch, 136, row, TEXTSELECT_96, NULL, numchanselect_cb, NULL);


    row += 24;
    GUI_CreateLabel(&gui->pwrlbl, 8, row, NULL, DEFAULT_FONT, _tr("Tx power"));
    GUI_CreateTextSelect(&gui->pwr, 136, row, TEXTSELECT_96, NULL, powerselect_cb, NULL);

    row += 20;
    if(Model.fixed_id == 0)
        strncpy(mp->fixed_id, _tr("None"), sizeof(mp->fixed_id));
    else
        sprintf(mp->fixed_id, "%d", (int)Model.fixed_id);
    GUI_CreateLabel(&gui->fixedidlbl, 8, row, NULL, DEFAULT_FONT, _tr("Fixed ID"));
    GUI_CreateButton(&gui->fixedid, 136, row, BUTTON_96x16, show_text_cb, 0x0000, fixedid_cb, mp->fixed_id);
    GUI_CreateButton(&gui->bind, 236, row, BUTTON_64x16, show_bindtext_cb, 0x0000, bind_cb, NULL);
    configure_bind_button();
}

/* Button callbacks */
static void _changename_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    GUI_RemoveObj(obj);
    if (callback_result == 1) {
        strcpy(Model.name, mp->tmpstr);
        //Save model info here so it shows up on the model page
        CONFIG_SaveModelIfNeeded();
    }
    PAGE_ModelInit(0);
}

static void _changename_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    PAGE_RemoveAllObjects();
    strcpy(mp->tmpstr, Model.name);
    callback_result = 1;
    GUI_CreateKeyboard(&gui->keyboard, KEYBOARD_ALPHA, mp->tmpstr, sizeof(Model.name)-1, _changename_done_cb, &callback_result);
}

static inline guiObject_t *_get_obj(int type, int objid)
{
    (void)objid;
    switch(type) {
        case ITEM_NUMCHAN: return (guiObject_t *)&gui->numch;
        case ITEM_TXPOWER: return (guiObject_t *)&gui->pwr;
        case ITEM_PROTO: return (guiObject_t *)&gui->bind;
        case ITEM_GUI: return (guiObject_t *)&gui->guits;
        case ITEM_PPMIN: return (guiObject_t *)&gui->ppm;
        default: return NULL;
    }
}
