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

// string too long for devo10, so define it separately for devo8 and devo10
static const char * const HELI_LABEL = _tr_noop("Helicopter");
static const char * const PLANE_LABEL = _tr_noop("Airplane");
static const char * const MULTI_LABEL = _tr_noop("Multirotor");

#include "../common/_model_page.c"

#if HAS_STANDARD_GUI
static const char *_mixermode_cb(guiObject_t *obj, int dir, void *data)
{
    const char *ret = mixermode_cb(obj, dir, data);
    if(Model.mixer_mode != mp->last_mixermode) {
        mp->last_mixermode = Model.mixer_mode;
        PAGE_RemoveHeader();
        PAGE_ShowHeader(PAGE_GetName(PAGEID_MODEL));
    }
    return ret;
}
#endif //HAS_STANDARD_GUI

void PAGE_ModelInit(int page)
{
    (void)page;

    mp->last_mixermode = Model.mixer_mode;
    mp->last_txpower = Model.tx_power;
    mp->file_state = 0;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MODEL));

    BeginGridLayout(9, 8) {
        GUI_CreateLabelBox(&gui->filelbl, Grid_XY(1, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("File"));
        GUI_CreateTextSelect(&gui->file, Grid_XY(1, 4), TEXTSELECT_96, file_press_cb, file_val_cb, NULL);

        GUI_CreateLabelBox(&gui->namelbl, Grid_XY(2, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Model name"));  // use the same naming convention for devo8 and devo10
        GUI_CreateButton(&gui->name, Grid_XY(2, 4), BUTTON_96x16, show_text_cb, _changename_cb, Model.name);
        GUI_CreateButton(&gui->icon, Grid_XY(2, 7), BUTTON_64x16, show_text_cb, changeicon_cb, _tr("Icon"));

        GUI_CreateLabelBox(&gui->typelbl, Grid_XY(3, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Model type"));
        GUI_CreateTextSelect(&gui->type, Grid_XY(3, 4), TEXTSELECT_96, type_press_cb, type_val_cb, NULL);

        GUI_CreateLabelBox(&gui->ppmlbl, Grid_XY(4, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("PPM In"));
        GUI_CreateTextSelect(&gui->ppm, Grid_XY(4, 4), TEXTSELECT_96, ppmin_press_cb, ppmin_select_cb, NULL);

        GUI_CreateLabelBox(&gui->protolbl, Grid_XY(5, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Protocol"));
        GUI_CreateTextSelect(&gui->proto, Grid_XY(5, 4), TEXTSELECT_96, proto_press_cb, protoselect_cb, NULL);

        GUI_CreateLabelBox(&gui->numchlbl, Grid_XY(6, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("# Channels"));
        GUI_CreateTextSelect(&gui->numch, Grid_XY(6, 4), TEXTSELECT_96, NULL, numchanselect_cb, NULL);

        GUI_CreateLabelBox(&gui->pwrlbl, Grid_XY(7, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Tx power"));
        GUI_CreateTextSelect(&gui->pwr, Grid_XY(7, 4), TEXTSELECT_96, NULL, powerselect_cb, NULL);

        if (Model.fixed_id == 0)
            strlcpy(mp->fixed_id, _tr("None"), sizeof(mp->fixed_id));
        else
            sprintf(mp->fixed_id, "%d", (int)Model.fixed_id);
        GUI_CreateLabelBox(&gui->fixedidlbl, Grid_XY(8, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Fixed ID"));
        GUI_CreateButton(&gui->fixedid, Grid_XY(8, 4), BUTTON_96x16, show_text_cb, fixedid_cb, mp->fixed_id);
        GUI_CreateButton(&gui->bind, Grid_XY(8, 7), BUTTON_64x16, show_bindtext_cb, bind_cb, NULL);

    #if HAS_STANDARD_GUI
        GUI_CreateLabelBox(&gui->guilbl, Grid_XY(9, 1), Grid_WH_Span(3, 1), &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Mixer GUI"));
        GUI_CreateTextSelect(&gui->guits, Grid_XY(9, 4), TEXTSELECT_96, NULL, _mixermode_cb, NULL);
    #endif
    }
    configure_bind_button();
}

/* Button callbacks */
static void _changename_done_cb(guiObject_t *obj, void *data)
{
    (void)data;
    GUI_RemoveObj(obj);
    if (callback_result == 1) {
        strlcpy(Model.name, tempstring, sizeof(Model.name));
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
    tempstring_cpy(Model.name);
    callback_result = 1;
    GUI_CreateKeyboard(&gui->keyboard, KEYBOARD_ALPHA, tempstring, sizeof(Model.name)-1, _changename_done_cb, &callback_result);
}

static inline guiObject_t *_get_obj(int type, int objid)
{
    (void)objid;
    switch(type) {
        case ITEM_NUMCHAN: return (guiObject_t *)&gui->numch;
        case ITEM_TXPOWER: return (guiObject_t *)&gui->pwr;
        case ITEM_PROTO: return (guiObject_t *)&gui->bind;
#if HAS_STANDARD_GUI
        case ITEM_GUI: return (guiObject_t *)&gui->guits;
#endif
        case ITEM_PPMIN: return (guiObject_t *)&gui->ppm;
        default: return NULL;
    }
}
