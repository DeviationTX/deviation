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

#include "../common/_model_config.c"

#define VIEW_ID 0
#define gui (&gui_objs.u.modelcfg)
#define guip (&gui_objs.u.modelproto)

static s16 view_origin_relativeY;
static s8 current_selected;

static u8 _action_cb(u32 button, u8 flags, void *data);
static void show_titlerow(const char *header)
{
    PAGE_ShowHeader(header);
    //u8 w = 40;
    // I don't think there is a need for the save button
    //GUI_CreateButtonPlateText(LCD_WIDTH - w -5, 0,
    //        w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, 0x0000, okcancel_cb, _tr("Save"));
}

void MODELPAGE_Config()
{
    PAGE_SetModal(1);
    PAGE_SetActionCB(_action_cb);
    show_titlerow(Model.type == 0 ? _tr("Helicopter") : _tr("Airplane"));

    // Even though there are just 4 rows here, still create a logical view for future expanding
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    current_selected = 0;

    u8 space = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - view_origin_absoluteY ,
        view_origin_absoluteX, view_origin_absoluteY);
    u8 w = 60;
    u8 x = 63;
    mp->total_items = 0;
    if (Model.type == 0) {
        u8 row = 0;
        GUI_CreateLabelBox(&gui->swash, GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("SwashType:"));
        guiObject_t *obj = GUI_CreateTextSelectPlate(&gui->swashsel, GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, swash_val_cb, NULL);
        GUI_SetSelected(obj);

        row += space;
        GUI_CreateLabelBox(&gui->ele, GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("ELE Inv:"));
        GUI_CreateTextSelectPlate(&gui->elesel, GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, swashinv_press_cb, swashinv_val_cb, (void *)1);

        row += space;
        GUI_CreateLabelBox(&gui->ail, GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("AIL Inv:"));
        GUI_CreateTextSelectPlate(&gui->ailsel, GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, swashinv_press_cb, swashinv_val_cb, (void *)2);

        row += space;
        GUI_CreateLabelBox(&gui->col, GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("COL Inv:"));
        GUI_CreateTextSelectPlate(&gui->colsel, GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, swashinv_press_cb, swashinv_val_cb, (void *)4);

        row += space;
        GUI_CreateLabelBox(&gui->emix, GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("ELE Mix:"));
        GUI_CreateTextSelectPlate(&gui->emixsel, GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, swashmix_val_cb, (void *)1);

        row += space;
        GUI_CreateLabelBox(&gui->amix, GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("AIL Mix:"));
        GUI_CreateTextSelectPlate(&gui->amixsel, GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, swashmix_val_cb, (void *)0);

        row += space;
        GUI_CreateLabelBox(&gui->cmix, GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("COL Mix:"));
        GUI_CreateTextSelectPlate(&gui->cmixsel, GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, swashmix_val_cb, (void *)2);
        mp->total_items = 7;
        GUI_CreateScrollbar(&gui->scroll, LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT- ITEM_HEIGHT, mp->total_items, NULL, NULL, NULL);
    }
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ModelInit(-1);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            PAGE_NavigateItems(-1, VIEW_ID, mp->total_items, &current_selected, &view_origin_relativeY, &gui->scroll);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            PAGE_NavigateItems(1, VIEW_ID, mp->total_items, &current_selected, &view_origin_relativeY, &gui->scroll);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

void MODELPROTO_Config()
{
    PAGE_SetModal(1);
    PAGE_SetActionCB(_action_cb);
    show_titlerow(ProtocolNames[Model.protocol]);

    proto_strs = PROTOCOL_GetOptions();

    // Even though there are just 4 rows here, still create a logical view for future expanding
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 space = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - view_origin_absoluteY ,
        view_origin_absoluteX, view_origin_absoluteY);
    u8 w = 60;
    u8 x = 63;


    u8 row = 0;
    int pos = 0;
    long idx = 0;
    while(idx < NUM_PROTO_OPTS) {
        if(proto_strs[pos] == NULL)
            break;
        GUI_CreateLabelBox(&guip->str[idx], GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, row),
                0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr(proto_strs[pos]));
        guiObject_t *obj = GUI_CreateTextSelectPlate(&guip->sel[idx], GUI_MapToLogicalView(VIEW_ID, x), GUI_MapToLogicalView(VIEW_ID, row),
                w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, proto_opt_cb, (void *)idx);
        if (idx == 0)
            GUI_SetSelected(obj);
        while(proto_strs[++pos])
            ;
        pos++;
        idx++;
        row += space;
    }
}
