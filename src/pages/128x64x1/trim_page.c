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

#include "../common/_trim_page.c"

#define gui (&gui_objs.u.trim)
#define guit (&gui_objs.u.trim2)
#define VIEW_ID 0
#define SUB_VIEW_ID 1

static u8 _action_cb(u32 button, u8 flags, void *data);
static u8 _sub_action_cb(u32 button, u8 flags, void *data);
static s8 selectedIdx = 0;
static s16 view_origin_relativeY = 0;
static u8 max_items = NUM_TRIMS * 2;

static void _show_page()
{
    PAGE_SetActionCB(_action_cb);
    guiObject_t *obj;
    //PAGE_ShowHeader(_tr("Trim")); // no title for devo10
    u8 w = 30;
    PAGE_ShowHeader(_tr("Input:"));
    GUI_CreateLabelBox(&gui->steplbl, 40, 0, 30, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Step:"));
    // no enought space in Devo10, so just display trim + in the 1st page
    //GUI_CreateLabelBox(w + 40, 0, 0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Trim -:"));
    GUI_CreateLabelBox(&gui->trimposlbl, 80, 0, 30, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Trim +:"));
    struct Trim *trim = MIXER_GetAllTrims();

    u8 space = ITEM_HEIGHT +1;
    // create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    GUI_SetupLogicalView(VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - space, view_origin_absoluteX, view_origin_absoluteY);
    u8 y = 0;
    for (u8 i = 0; i < NUM_TRIMS; i++) {
        GUI_CreateButtonPlateText(&gui->src[i], GUI_MapToLogicalView(VIEW_ID, 0), GUI_MapToLogicalView(VIEW_ID, y), w, ITEM_HEIGHT,
                &DEFAULT_FONT, trimsource_name_cb, 0x0000, _edit_cb, (void *)((long)i));
        //GUI_CreateLabel(72, 24*i + 66, NULL, DEFAULT_FONT, (void *)INPUT_ButtonName(trim[i].neg));
        GUI_CreateTextSelectPlate(&gui->item[i], GUI_MapToLogicalView(VIEW_ID, 31), GUI_MapToLogicalView(VIEW_ID, y),
                40, ITEM_HEIGHT, &TINY_FONT,  NULL, set_trimstep_cb, &trim[i].step);
        GUI_CreateLabelBox(&gui->name[i], GUI_MapToLogicalView(VIEW_ID, 75), GUI_MapToLogicalView(VIEW_ID, y), 50, ITEM_HEIGHT,
                &DEFAULT_FONT, NULL, NULL,  (void *)INPUT_ButtonName(trim[i].pos));

        y += space;
    }
    GUI_CreateScrollbar(&gui->scroll, LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT- ITEM_HEIGHT, max_items, NULL, NULL, NULL);

    GUI_SetSelected((guiObject_t *)&gui->item[selectedIdx]);
    // when entering this page from its children page, we need to scroll the view to its previous position
    obj = GUI_GetSelected();
    if (!GUI_IsObjectInsideCurrentView(VIEW_ID, obj))
        GUI_SetRelativeOrigin(VIEW_ID, 0, view_origin_relativeY);
    GUI_SetScrollbar(&gui->scroll, selectedIdx);
}

static void _edit_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    PAGE_SetActionCB(_sub_action_cb);
    struct Trim *trim = MIXER_GetAllTrims();
    PAGE_SetModal(1);
    tp->index = (long)data;
    tp->trim = trim[tp->index];
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(""); // to draw a line only

    u8 w = 50;
    GUI_CreateButtonPlateText(&guit->save, LCD_WIDTH - w -1, 0, w, ITEM_HEIGHT,
            &DEFAULT_FONT, NULL, 0x0000, okcancel_cb, (void *)_tr("Save"));

    u8 space = ITEM_HEIGHT + 1;
    // Even though we can draw all the 4 rows in a page, still create in view for future expansion
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = space;
    GUI_SetupLogicalView(SUB_VIEW_ID, 0, 0, LCD_WIDTH -5, LCD_HEIGHT - space, view_origin_absoluteX, view_origin_absoluteY);

    u8 y = 0;
    u8 x = 60;
    w = 63;
    GUI_CreateLabelBox(&guit->srclbl, GUI_MapToLogicalView(SUB_VIEW_ID, 0), GUI_MapToLogicalView(SUB_VIEW_ID, y), 0, ITEM_HEIGHT,
            &DEFAULT_FONT, NULL, NULL,  (void *)_tr("Input:"));
    guiObject_t *obj1 = GUI_CreateTextSelectPlate(&guit->src, GUI_MapToLogicalView(SUB_VIEW_ID, x), GUI_MapToLogicalView(SUB_VIEW_ID, y),
            w, ITEM_HEIGHT, &DEFAULT_FONT,  NULL, set_source_cb, &tp->trim.src);
    GUI_SetSelected(obj1);

    y += space;
    GUI_CreateLabelBox(&guit->trimneglbl, GUI_MapToLogicalView(SUB_VIEW_ID, 0), GUI_MapToLogicalView(SUB_VIEW_ID, y), 0, ITEM_HEIGHT,
            &DEFAULT_FONT, NULL, NULL,  (void *)_tr("Trim -:"));
    GUI_CreateTextSelectPlate(&guit->trimneg, GUI_MapToLogicalView(SUB_VIEW_ID, x-10), GUI_MapToLogicalView(SUB_VIEW_ID, y),
            w +10, ITEM_HEIGHT, &DEFAULT_FONT,  NULL, set_trim_cb, &tp->trim.neg);

    y += space;
    GUI_CreateLabelBox(&guit->trimposlbl, GUI_MapToLogicalView(SUB_VIEW_ID, 0), GUI_MapToLogicalView(SUB_VIEW_ID, y), 0, ITEM_HEIGHT,
            &DEFAULT_FONT, NULL, NULL,  (void *)_tr("Trim +:"));
    GUI_CreateTextSelectPlate(&guit->trimpos, GUI_MapToLogicalView(SUB_VIEW_ID, x-10), GUI_MapToLogicalView(SUB_VIEW_ID, y),
            w +10, ITEM_HEIGHT, &DEFAULT_FONT,  NULL,  set_trim_cb, &tp->trim.pos);

    y += space;
    GUI_CreateLabelBox(&guit->steplbl, GUI_MapToLogicalView(SUB_VIEW_ID, 0), GUI_MapToLogicalView(SUB_VIEW_ID, y), 0, ITEM_HEIGHT,
            &DEFAULT_FONT, NULL, NULL,  (void *)_tr("Trim Step:"));
    GUI_CreateTextSelectPlate(&guit->step, GUI_MapToLogicalView(SUB_VIEW_ID, x), GUI_MapToLogicalView(SUB_VIEW_ID, y),
            w, ITEM_HEIGHT, &DEFAULT_FONT,  NULL,  set_trimstep_cb, &tp->trim.step);
}


static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }  else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            PAGE_NavigateItems(-1, VIEW_ID, max_items, &selectedIdx, &view_origin_relativeY, &gui->scroll);
        }  else if (CHAN_ButtonIsPressed(button,BUT_DOWN)) {
            PAGE_NavigateItems(1, VIEW_ID, max_items, &selectedIdx, &view_origin_relativeY, &gui->scroll);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static u8 _sub_action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if (flags & BUTTON_PRESS || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_TrimInit(0);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
