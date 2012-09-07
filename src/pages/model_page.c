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

static struct model_page * const mp = &pagemem.u.model_page;

static void changename_cb(guiObject_t *obj, const void *data);
static void fixedid_cb(guiObject_t *obj, const void *data);
static void bind_cb(guiObject_t *obj, const void *data);
static void configure_bind_button();
static const char *type_val_cb(guiObject_t *obj, int dir, void *data);
static void type_press_cb(guiObject_t *obj, void *data);
static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data);
static const char *powerselect_cb(guiObject_t *obj, int dir, void *data);
static const char *protoselect_cb(guiObject_t *obj, int dir, void *data);
static const char *file_val_cb(guiObject_t *obj, int dir, void *data);
static void file_press_cb(guiObject_t *obj, void *data);
static void changeicon_cb(guiObject_t *obj, const void *data);

const char *show_text_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return (const char *)data;
}

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
    GUI_CreateButton(136, row, BUTTON_96x16, show_text_cb, 0x0000, changename_cb, Model.name);
    GUI_CreateButton(236, row, BUTTON_64x16, show_text_cb, 0x0000, changeicon_cb, _tr("Icon"));

    row += 20;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Model Type:"));
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, type_press_cb, type_val_cb, NULL);

    row += 32;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Protocol:"));
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, protoselect_cb, NULL);

    row += 20;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, _tr("Number of Channels:"));
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, numchanselect_cb, NULL);


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
    mp->obj = GUI_CreateButton(236, row, BUTTON_64x16, show_text_cb, 0x0000, bind_cb, _tr("Bind"));
    configure_bind_button();
}

void PAGE_ModelEvent()
{
}

/* Button callbacks */
static void changename_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    GUI_RemoveObj(obj);
    //Save model info here so it shows up on the model page
    CONFIG_SaveModelIfNeeded();
    PAGE_ModelInit(0);
}
static void changename_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    PAGE_RemoveAllObjects();
    GUI_CreateKeyboard(KEYBOARD_ALPHA, Model.name, sizeof(Model.name)-1, changename_done_cb, NULL);
}

static void fixedid_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    Model.fixed_id = atoi(mp->fixed_id);
    GUI_RemoveObj(obj);
    PAGE_ModelInit(0);
}
static void fixedid_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    if(Model.fixed_id == 0)
        mp->fixed_id[0] = 0;
    PAGE_RemoveAllObjects();
    GUI_CreateKeyboard(KEYBOARD_NUM, mp->fixed_id, 999999, fixedid_done_cb, NULL);
}

static void bind_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    PROTOCOL_Bind();
}

static void configure_bind_button()
{
    GUI_SetHidden(mp->obj, PROTOCOL_AutoBindEnabled());
}

/* Text Select Callback */
static const char *type_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.type = GUI_TextSelectHelper(Model.type, 0, 1, dir, 1, 1, NULL);
    GUI_TextSelectEnablePress(obj, Model.type == 0);

    switch (Model.type) {
        case 0: return _tr("Helicopter");
        default: return _tr("Airplane");
    }
}
void type_press_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    if(Model.type == 0) {
        PAGE_RemoveAllObjects();
        MODELPAGE_Config();
    }
}

static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.num_channels = GUI_TextSelectHelper(Model.num_channels, 1, NUM_OUT_CHANNELS, dir, 1, 1, NULL);
    sprintf(mp->tmpstr, "%d", Model.num_channels);
    return mp->tmpstr;
}

static const char *powerselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.tx_power = GUI_TextSelectHelper(Model.tx_power, TXPOWER_100uW, TXPOWER_LAST-1, dir, 1, 1, NULL);
    return RADIO_TX_POWER_VAL[Model.tx_power];
}
static const char *protoselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Model.protocol = GUI_TextSelectHelper(Model.protocol, PROTOCOL_NONE, PROTOCOL_COUNT-1, dir, 1, 1, &changed);
    if (changed)
        configure_bind_button();
    return ProtocolNames[Model.protocol];
}
static const char *file_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    mp->file_state = GUI_TextSelectHelper(mp->file_state, 0, 3, dir, 1, 1, NULL);
    if (mp->file_state == 0)
        return _tr("Load...");
    else if (mp->file_state == 1)
        return _tr("Copy To...");
    else if (mp->file_state == 2)
        return _tr("Template..");
    else if (mp->file_state == 3)
        return _tr("Reset");
    else
        return "";
}

static void file_press_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    if (mp->file_state == 3) {
        CONFIG_ResetModel();
        CONFIG_SaveModelIfNeeded();
        GUI_RedrawAllObjects();
    } else {
        PAGE_SetModal(1);
        MODELPage_ShowLoadSave(mp->file_state, PAGE_ModelInit);
    }
}

static void changeicon_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    MODELPage_ShowLoadSave(3, PAGE_ModelInit);
}
