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
#include "mixer_standard.h"
#include "standard/standard.h"

#include <stdlib.h>

static const char * const HELI_LABEL = _tr_noop("Heli");
static const char * const PLANE_LABEL = _tr_noop("Plane");
static const char * const MULTI_LABEL = _tr_noop("Multi");
#include "../common/_model_page.c"

static unsigned _action_cb(u32 button, unsigned flags, void *data);

static u16 current_selected = 0;


static const char * show_icontext_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    unsigned int i;
    if(! Model.icon[0])
        return _tr("Default");
    tempstring_cpy(Model.icon+9);
    for(i = 0; i < strlen(tempstring); i++) {
        if(tempstring[i] == '.') {
            tempstring[i] = '\0';
            break;
        }
    }
    return tempstring;
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void) data;
    if(col == 0 && gui->col1[relrow].ts.header.Type == TextSelect) {
        return (guiObject_t *)&gui->col1[relrow];
    }
    return (guiObject_t *)&gui->col2[relrow];
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void) data;
    u8 w = 59;
    u8 x = 63;
    u8 ts_x = 63;
    int count = 0;
    const void *label = NULL;
    void *ts_tgl = NULL;
    void *ts_value = NULL;
    void *but_txt = NULL;
    void *but_tgl = NULL;
    void *but_data = NULL;

    switch(absrow) {
        case ITEM_FILE:
            label = _tr_noop("File");
            ts_tgl = file_press_cb; ts_value = file_val_cb;
            break;
        case ITEM_NAME:
            label = _tr_noop("Model name");
            but_txt = show_text_cb; but_tgl = _changename_cb; but_data = Model.name;
            break;
        case ITEM_ICON:
            label = _tr_noop("Icon");
            but_txt = show_icontext_cb; but_tgl = changeicon_cb;
            break;
        case ITEM_TYPE:
            label = _tr_noop("Model type");
            ts_tgl = type_press_cb; ts_value = type_val_cb;
            break;
        case ITEM_TXPOWER:
            label = _tr_noop("Tx power");
            ts_value = powerselect_cb;
            break;
        case ITEM_PPMIN:
            label = _tr_noop("PPM In");
            ts_tgl = ppmin_press_cb; ts_value = ppmin_select_cb;
            break;
        case ITEM_PROTO:
            ts_tgl = proto_press_cb; ts_value = protoselect_cb; ts_x = 0;
            but_txt = show_bindtext_cb; but_tgl = bind_cb;
            break;
        case ITEM_FIXEDID:
            label = _tr_noop("Fixed ID");
            but_txt = show_text_cb; but_tgl = fixedid_cb; but_data = mp->fixed_id;
            break;
        case ITEM_NUMCHAN:
            label = _tr_noop("# Channels");
            ts_value = numchanselect_cb;
            break;
#if HAS_STANDARD_GUI
        case ITEM_GUI:
            label = _tr_noop("Mixer GUI");
            ts_value = mixermode_cb;
            break;
#endif
    }
    if (label)
        GUI_CreateLabelBox(&gui->col1[relrow].label, 0, y,
           0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr(label));
    if (ts_value) {
        GUI_CreateTextSelectPlate(but_txt ? &gui->col1[relrow].ts : &gui->col2[relrow].ts, ts_x, y,
            w, LINE_HEIGHT, &DEFAULT_FONT, ts_tgl, ts_value, NULL);
        count++;
    }
    if (but_txt) {
        GUI_CreateButtonPlateText(&gui->col2[relrow].but, x, y,
            w, LINE_HEIGHT, &DEFAULT_FONT, but_txt, 0x0000, but_tgl, but_data);
        count++;
    }
    return count;
}
        
void PAGE_ModelInit(int page)
{
    (void)page;
    //if (page < 0 && current_selected > 0) // enter this page from childen page , so we need to get its previous mp->current_selected item
    //    page = current_selected;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    memset(gui, 0, sizeof(struct modelpage_obj));
    mp->file_state = 0;
    mp->last_txpower = Model.tx_power;
    PAGE_ShowHeader(_tr("Model setup")); // using the same name as related menu item to reduce language strings

    if(Model.fixed_id == 0)
        strlcpy(mp->fixed_id, _tr("None"), sizeof(mp->fixed_id));
    else
        sprintf(mp->fixed_id, "%d", (int)Model.fixed_id);

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, ITEM_LAST, row_cb, getobj_cb, NULL, NULL);

    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}

static void _changename_done_cb(guiObject_t *obj, void *data)  // devo8 doesn't handle cancel/discard properly,
{
    (void)obj;
    (void)data;
    GUI_RemoveObj(obj);
    if (callback_result == 1) {  // only change name when DONE is hit, otherwise, discard the change
        strlcpy(Model.name, (const char *)tempstring, sizeof(Model.name));
        //Save model info here so it shows up on the model page
        CONFIG_SaveModelIfNeeded();
    }
    PAGE_ModelInit(-1);
}

static void _changename_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_SetModal(1);
    PAGE_RemoveAllObjects();
    tempstring_cpy((const char *)Model.name); // Don't change model name directly
    GUI_CreateKeyboard(&gui->keyboard, KEYBOARD_ALPHA, tempstring, 20, // no more than 20 chars is allowed for model name
            _changename_done_cb, (void *)&callback_result);
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        PAGE_ModelExit();
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

void PAGE_ModelExit()
{
    current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
}

static inline guiObject_t *_get_obj(int type, int objid)
{
    return (guiObject_t *)GUI_GetScrollableObj(&gui->scrollable, type, objid);
}
