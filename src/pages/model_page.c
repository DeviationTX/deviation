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

static void loadsave_cb(guiObject_t *obj, void *data);
static void changename_cb(guiObject_t *obj, void *data);
static void fixedid_cb(guiObject_t *obj, void *data);
static const char *typeselect_cb(guiObject_t *obj, int dir, void *data);
static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data);
static const char *modeselect_cb(guiObject_t *obj, int dir, void *data);
static const char *powerselect_cb(guiObject_t *obj, int dir, void *data);
static const char *protoselect_cb(guiObject_t *obj, int dir, void *data);

void PAGE_ModelInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    GUI_CreateLabel(8, 10, NULL, TITLE_FONT, "Model");

    GUI_CreateButton(90, 4, BUTTON_96, "Load", 0x0000, loadsave_cb, (void *)0L);
    GUI_CreateButton(188, 4, BUTTON_96, "Save As", 0x0000, loadsave_cb, (void *)1L);

    GUI_CreateLabel(8, 48, NULL, DEFAULT_FONT, "Model Name:");
    GUI_CreateButton(136, 48, BUTTON_96x16, Model.name, 0x0000, changename_cb, NULL);

    GUI_CreateLabel(8, 72, NULL, DEFAULT_FONT, "Model Type:");
    GUI_CreateTextSelect(136, 72, TEXTSELECT_96, 0x0000, NULL, typeselect_cb, NULL);

    GUI_CreateLabel(8, 96, NULL, DEFAULT_FONT, "Number of Channels:");
    GUI_CreateTextSelect(136, 96, TEXTSELECT_96, 0x0000, NULL, numchanselect_cb, NULL);

    GUI_CreateLabel(8, 120, NULL, DEFAULT_FONT, "Mode:");
    GUI_CreateTextSelect(136, 120, TEXTSELECT_96, 0x0000, NULL, modeselect_cb, NULL);

    GUI_CreateLabel(8, 144, NULL, DEFAULT_FONT, "Tx Power:");
    GUI_CreateTextSelect(136, 144, TEXTSELECT_96, 0x0000, NULL, powerselect_cb, NULL);

    GUI_CreateLabel(8, 168, NULL, DEFAULT_FONT, "Protocol:");
    GUI_CreateTextSelect(136, 168, TEXTSELECT_96, 0x0000, NULL, protoselect_cb, NULL);

    if(Model.fixed_id == 0)
        sprintf(mp->fixed_id, "None");
    else
        sprintf(mp->fixed_id, "%d", Model.fixed_id);
    GUI_CreateLabel(8, 192, NULL, DEFAULT_FONT, "Fixed ID:");
    GUI_CreateButton(136, 192, BUTTON_96x16, mp->fixed_id, 0x0000, fixedid_cb, NULL);
}

void PAGE_ModelEvent()
{
}

/* Button callbacks */
static void loadsave_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    long loadsave = (long)data;
    PAGE_SetModal(1);
    MODELPage_ShowLoadSave(loadsave, PAGE_ModelInit);
}

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
    GUI_RemoveAllObjects();
    GUI_CreateKeyboard(KEYBOARD_CHAR, Model.name, sizeof(Model.name)-1, changename_done_cb, NULL);
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
    GUI_RemoveAllObjects();
    GUI_CreateKeyboard(KEYBOARD_NUM, mp->fixed_id, 6, fixedid_done_cb, NULL);
}

/* Text Select Callback */
static const char *typeselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 changed;
    Model.type = GUI_TextSelectHelper(Model.type, 0, 1, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(obj);
    }
    switch (Model.type) {
        case 0: return "Helicopter";
        default: return "Airplane";
    }
}

static const char *numchanselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 changed;
    Model.num_channels = GUI_TextSelectHelper(Model.num_channels, 1, NUM_CHANNELS, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(obj);
    }
    sprintf(mp->tmpstr, "%d", Model.num_channels);
    return mp->tmpstr;
}
static const char *modeselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 changed;
    Model.mode = GUI_TextSelectHelper(Model.mode, MODE_1, MODE_4, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(obj);
    }
    sprintf(mp->tmpstr, "Mode %d", Model.mode + 1);
    return mp->tmpstr;
}

static const char *powerselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 changed;
    Model.tx_power = GUI_TextSelectHelper(Model.tx_power, TXPOWER_300uW, TXPOWER_100mW, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(obj);
    }
    return RADIO_TX_POWER_VAL[Model.tx_power];
}
static const char *protoselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 changed;
    Model.protocol = GUI_TextSelectHelper(Model.protocol, PROTOCOL_NONE, PROTOCOL_J6PRO, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(obj);
    }
    return RADIO_PROTOCOL_VAL[Model.protocol];
}
