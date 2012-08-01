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
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include <stdlib.h>

static struct model_page * const mp = &pagemem.u.model_page;

static void changename_cb(guiObject_t *obj, void *data);
static void fixedid_cb(guiObject_t *obj, void *data);
static const char *type_val_cb(guiObject_t *obj, int dir, void *data);
static void type_press_cb(guiObject_t *obj, void *data);
static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data);
static const char *modeselect_cb(guiObject_t *obj, int dir, void *data);
static const char *powerselect_cb(guiObject_t *obj, int dir, void *data);
static const char *protoselect_cb(guiObject_t *obj, int dir, void *data);
static const char *file_val_cb(guiObject_t *obj, int dir, void *data);
static void file_press_cb(guiObject_t *obj, void *data);

const char *show_text_cb(guiObject_t *obj, void *data)
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
    PAGE_ShowHeader("Model");

    row = 40;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "File:");
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, file_press_cb, file_val_cb, NULL);


    row += 32;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "Model Name:");
    GUI_CreateButton(136, row, BUTTON_96x16, show_text_cb, 0x0000, changename_cb, Model.name);

    row += 20;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "Model Type:");
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, type_press_cb, type_val_cb, NULL);

    row += 20;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "Mode:");
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, modeselect_cb, NULL);


    row += 32;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "Protocol:");
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, protoselect_cb, NULL);

    row += 20;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "Number of Channels:");
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, numchanselect_cb, NULL);


    row += 32;
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "Tx Power:");
    GUI_CreateTextSelect(136, row, TEXTSELECT_96, 0x0000, NULL, powerselect_cb, NULL);

    row += 20;
    if(Model.fixed_id == 0)
        sprintf(mp->fixed_id, "None");
    else
        sprintf(mp->fixed_id, "%d", Model.fixed_id);
    GUI_CreateLabel(8, row, NULL, DEFAULT_FONT, "Fixed ID:");
    GUI_CreateButton(136, row, BUTTON_96x16, show_text_cb, 0x0000, fixedid_cb, mp->fixed_id);
}

void PAGE_ModelEvent()
{
}

/* Button callbacks */
static void changename_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    GUI_RemoveObj(obj);
    PAGE_ModelInit(0);
}
static void changename_cb(guiObject_t *obj, void *data)
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
static void fixedid_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    if(Model.fixed_id == 0)
        mp->fixed_id[0] = 0;
    PAGE_RemoveAllObjects();
    GUI_CreateKeyboard(KEYBOARD_NUM, mp->fixed_id, 6, fixedid_done_cb, NULL);
}

/* Text Select Callback */
static const char *type_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.type = GUI_TextSelectHelper(Model.type, 0, 1, dir, 1, 1, NULL);
    GUI_TextSelectEnablePress(obj, Model.type == 0);

    switch (Model.type) {
        case 0: return "Helicopter";
        default: return "Airplane";
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
    Model.num_channels = GUI_TextSelectHelper(Model.num_channels, 1, NUM_CHANNELS, dir, 1, 1, NULL);
    sprintf(mp->tmpstr, "%d", Model.num_channels);
    return mp->tmpstr;
}
static const char *modeselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.mode = GUI_TextSelectHelper(Model.mode, MODE_1, MODE_4, dir, 1, 1, NULL);
    sprintf(mp->tmpstr, "Mode %d", Model.mode + 1);
    return mp->tmpstr;
}

static const char *powerselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.tx_power = GUI_TextSelectHelper(Model.tx_power, TXPOWER_300uW, TXPOWER_100mW, dir, 1, 1, NULL);
    return RADIO_TX_POWER_VAL[Model.tx_power];
}
static const char *protoselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Model.protocol = GUI_TextSelectHelper(Model.protocol, PROTOCOL_NONE, PROTOCOL_J6PRO, dir, 1, 1, NULL);
    return RADIO_PROTOCOL_VAL[Model.protocol];
}
static const char *file_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    mp->file_state = GUI_TextSelectHelper(mp->file_state, 0, 3, dir, 1, 1, NULL);
    if (mp->file_state == 0)
        return "Load...";
    else if (mp->file_state == 1)
        return "Copy To...";
    else if (mp->file_state == 2)
        return "Template..";
    else if (mp->file_state == 3)
        return "Reset";
    else
        return "";
}

static void file_press_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    if (mp->file_state == 3) {
        CONFIG_ResetModel();
        GUI_RedrawAllObjects();
    } else {
        MODELPage_ShowLoadSave(mp->file_state, PAGE_ModelInit);
    }
}
