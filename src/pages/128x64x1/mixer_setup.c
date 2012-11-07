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
#include "config/model.h"

#include "../common/_mixer_setup.c"

#define FIRST_PAGE_ITEM_IDX  2  // 0 is the template obj and 1 is the button obj
#define LEFT_VIEW_WIDTH  60
#define LEFT_VIEW_ID 0
#define RIGHT_VIEW_ID 1
#define RIGHT_VIEW_HEIGHT 49
static s8 current_selected_item;

static u8 action_cb(u32 button, u8 flags, void *data);
static void _show_expo_dr_switch(u8 switch_no);
static void _sourceselect_cb(guiObject_t *obj, void *data);
static const char *_set_drsource_cb(guiObject_t *obj, int dir, void *data);
static u8 _action_cb_switch(u32 button, u8 flags, void *data);

static void _show_titlerow()
{
    PAGE_SetActionCB(action_cb);
    mp->entries_per_page = 2;
    memset(mp->itemObj, 0, sizeof(mp->itemObj));

    mp->labelDesc.style = LABEL_UNDERLINE;
    GUI_CreateLabelBox(0, 0 , LCD_WIDTH, ITEM_HEIGHT, &mp->labelDesc,
            MIXPAGE_ChanNameProtoCB, NULL, (void *)((long)mp->cur_mixer->dest));
    u8 x =40;
    u8 w = 50;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[0] = GUI_CreateTextSelectPlate(x, 0,  w, ITEM_HEIGHT, &mp->labelDesc, NULL, templatetype_cb, (void *)((long)mp->channel));
    GUI_SetSelected(mp->itemObj[0]);
    w = 30;
    mp->itemObj[1] = GUI_CreateButton(LCD_WIDTH - w, 0, BUTTON_DEVO10, NULL, 0, okcancel_cb, (void *)_tr("Save"));
    GUI_CustomizeButton(mp->itemObj[1] , &mp->labelDesc, w, ITEM_HEIGHT);

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 h = LCD_HEIGHT - view_origin_absoluteY ;
    GUI_SetupLogicalView(LEFT_VIEW_ID, 0, 0, LEFT_VIEW_WIDTH, h, view_origin_absoluteX, view_origin_absoluteY);
}

static void _show_simple()
{
    mp->max_scroll = FIRST_PAGE_ITEM_IDX;
    current_selected_item = -1;

    u8 x = 0;
    u8 space = ITEM_HEIGHT + 1;
    u8 w = LEFT_VIEW_WIDTH;
    u8 y = 0;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    mp->firstObj = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Src:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, sourceselect_cb,
            set_source_cb, &mp->mixer[0].src);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Curve:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, curveselect_cb, set_curvename_cb, &mp->mixer[0]);

    y += space;  // out of current logical view, won't show by default
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL,_tr("Scale:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL,
            set_number100_cb, &mp->mixer[0].scalar);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Offset:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL,
            set_number100_cb, &mp->mixer[0].offset);

    // The following items are not draw in the logical view;
    mp->graphs[0] = GUI_CreateXYGraph(77, LCD_HEIGHT - RIGHT_VIEW_HEIGHT - 1, RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb,
                              &mp->mixer[0]);

    y = ITEM_HEIGHT;
    x = LEFT_VIEW_WIDTH + ARROW_WIDTH;
    u8 h = LCD_HEIGHT - y ;
    mp->max_scroll -= FIRST_PAGE_ITEM_IDX;
    mp->scroll_bar = GUI_CreateScrollbar(x, y, h, mp->max_scroll, NULL, NULL, NULL);
}

static void _show_complex()
{
    mp->max_scroll = FIRST_PAGE_ITEM_IDX;
    current_selected_item = -1;

    u8 x = 0;
    u8 space = ITEM_HEIGHT + 1;
    u8 w = LEFT_VIEW_WIDTH;
    u8 y = 0;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    //Row 1
    if (! mp->expoObj[0]) {
        mp->firstObj = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
                &mp->labelDesc, NULL, NULL, _tr("Mixers:"));
        y += space;
        mp->labelDesc.style = LABEL_CENTER;
        mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_nummixers_cb, NULL);

        y += space;
        mp->labelDesc.style = LABEL_LEFTCENTER;
        GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
                        &mp->labelDesc, NULL, NULL, _tr("Page:"));
        y += space;
        mp->labelDesc.style = LABEL_CENTER;
        mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &mp->labelDesc, reorder_cb, set_mixernum_cb, NULL);
    } else {
        GUI_RemoveHierObjects(mp->expoObj[0]);
    }

    //Row 2
    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    mp->expoObj[0] = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Switch:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, sourceselect_cb, set_source_cb, &mp->cur_mixer->sw);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Mux:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_mux_cb, NULL);


    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Src:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, sourceselect_cb, set_source_cb, &mp->cur_mixer->src);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Curve:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, curveselect_cb, set_curvename_cb, mp->cur_mixer);


    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Scale:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_number100_cb, &mp->cur_mixer->scalar);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Offset:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_number100_cb, &mp->cur_mixer->offset);

    y += space;
    mp->trimObj = GUI_CreateButton(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), BUTTON_DEVO10, show_trim_cb, 0x0000, toggle_trim_cb, NULL);
    mp->itemObj[mp->max_scroll++] = mp->trimObj;
    GUI_CustomizeButton(mp->trimObj, &mp->labelDesc, w, ITEM_HEIGHT);
    if (! MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
        GUI_SetHidden(mp->trimObj, 1);


    // The following items are not draw in the logical view;
    mp->graphs[1] = GUI_CreateBarGraph(LEFT_VIEW_WIDTH +10, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1, 5, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL, eval_chan_cb, NULL);
    mp->graphs[0] = GUI_CreateXYGraph(77, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1, RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                                  CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                                  CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                                  0, 0, eval_mixer_cb, curpos_cb, touch_cb, mp->cur_mixer);

    y = ITEM_HEIGHT;
    x = LEFT_VIEW_WIDTH + ARROW_WIDTH;
    u8 h = LCD_HEIGHT - y ;
    mp->max_scroll -= FIRST_PAGE_ITEM_IDX;
    mp->scroll_bar = GUI_CreateScrollbar(x, y, h, mp->max_scroll, NULL, NULL, NULL);
}

static void _show_expo_dr()
{
    mp->max_scroll = FIRST_PAGE_ITEM_IDX;
    current_selected_item = -1;

    sync_mixers();

    u8 x = 0;
    u8 space = ITEM_HEIGHT + 1;
    u8 w = LEFT_VIEW_WIDTH;
    u8 y = 0;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    mp->firstObj = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Src:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, sourceselect_cb, set_source_cb, &mp->mixer[0].src);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("High-Rate"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc,  curveselect_cb, set_curvename_cb, &mp->mixer[0]);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Scale:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc,  NULL, set_number100_cb, &mp->mixer[0].scalar);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Switch1"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &mp->labelDesc,  _sourceselect_cb, _set_drsource_cb, (void *)((long)1));//&mp->mixer[1].sw);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Switch2"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &mp->labelDesc,  _sourceselect_cb, _set_drsource_cb, (void *)((long)2)); //&mp->mixer[2].sw);

    GUI_SetupLogicalView(RIGHT_VIEW_ID, 0, 0, RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT, 77, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1);
    // The following items are draw in the right logical view,
    // the right view will be scroll up/down by changed the switch 1/2 options
    y = 0;
    mp->graphs[0] = GUI_CreateXYGraph(GUI_MapToLogicalView(RIGHT_VIEW_ID, 0) ,
                            GUI_MapToLogicalView(RIGHT_VIEW_ID, y), RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0]);
    y += RIGHT_VIEW_HEIGHT;
    mp->graphs[1] = GUI_CreateXYGraph(GUI_MapToLogicalView(RIGHT_VIEW_ID, 0) ,
                            GUI_MapToLogicalView(RIGHT_VIEW_ID, y), RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[1]);
    y += RIGHT_VIEW_HEIGHT;
    mp->graphs[2] = GUI_CreateXYGraph(GUI_MapToLogicalView(RIGHT_VIEW_ID, 0) ,
                            GUI_MapToLogicalView(RIGHT_VIEW_ID, y), RIGHT_VIEW_HEIGHT , RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[2]);

    // The following items are not draw in the logical view;
    y = ITEM_HEIGHT;
    x = LEFT_VIEW_WIDTH + ARROW_WIDTH;
    u8 h = LCD_HEIGHT - y ;
    mp->max_scroll -= FIRST_PAGE_ITEM_IDX;
    mp->scroll_bar = GUI_CreateScrollbar(x, y, h, mp->max_scroll, NULL, NULL, NULL);
}

static const char *_set_drsource_cb(guiObject_t *obj, int dir, void *data)
{
    u8 switch_no = (long)data;
    return set_drsource_cb(obj, dir, &mp->mixer[switch_no].sw);
}

static void _sourceselect_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    //sourceselect_cb(obj, data);
    u8 switch_no = (long)data;
    if (mp->mixer[switch_no].sw != 0)
        _show_expo_dr_switch(switch_no);
}

static const char *_title_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 switch_no = (long)data;
    sprintf(mp->tmpstr, "Switch%d", switch_no);
    return _tr(mp->tmpstr);
}

static void _show_expo_dr_switch(u8 switch_no)  // switch_no = 1 or 2
{
    GUI_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb_switch); // don't change mp->itemObj , must keep expo_dr page items

    mp->labelDesc.style = LABEL_UNDERLINE;
    GUI_CreateLabelBox(0, 0 , LCD_WIDTH, ITEM_HEIGHT, &mp->labelDesc,
            _title_cb, NULL, (void *)(long)switch_no);

    u8 w = 48;
    mp->labelDesc.style = LABEL_CENTER;
    guiObject_t *obj = GUI_CreateTextSelectPlate(LCD_WIDTH - w -5, 0, w, ITEM_HEIGHT, &mp->labelDesc,
            sourceselect_cb, set_drsource_cb, &mp->mixer[switch_no].sw);
    GUI_SetSelected(obj);

    /* w = 30;  // no need the save button
    obj = GUI_CreateButton(LCD_WIDTH - w, 0, BUTTON_DEVO10, NULL, 0, _save_switch_cb, (void *)_tr("Save"));
    GUI_CustomizeButton(obj , &mp->labelDesc, w, ITEM_HEIGHT);*/

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 h = LCD_HEIGHT - view_origin_absoluteY ;
    GUI_SetupLogicalView(LEFT_VIEW_ID, 0, 0, LEFT_VIEW_WIDTH, h, view_origin_absoluteX, view_origin_absoluteY);

    u8 x = 0;
    u8 space = ITEM_HEIGHT + 1;
    w = LEFT_VIEW_WIDTH;
    u8 y = 0;
    u8 idx = (switch_no -1) * 4;
    mp->expoObj[idx] = GUI_CreateButton(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            BUTTON_DEVO10, show_rate_cb, 0x0000, toggle_link_cb, (void *)(switch_no-1));
    GUI_CustomizeButton(mp->expoObj[idx], &mp->labelDesc, w, ITEM_HEIGHT);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    mp->expoObj[idx +1] = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, NULL, _tr("Linked"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->expoObj[idx +2] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc,  curveselect_cb, set_curvename_cb, &mp->mixer[switch_no]);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    mp->expoObj[9] = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Scale:"));
    y += space;
    mp->labelDesc.style = LABEL_CENTER;
    mp->expoObj[idx +3] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc,  NULL, set_number100_cb, &mp->mixer[switch_no].scalar);

    mp->graphs[switch_no] = GUI_CreateXYGraph(77, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1, RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[switch_no]);
    //Enable/Disable the relevant widgets
    update_rate_widgets(switch_no - 1);
}

static void navigate_items(s8 direction)
{
    guiObject_t *obj = GUI_GetSelected();
    u8 last_item = mp->max_scroll + FIRST_PAGE_ITEM_IDX -1;
    if (direction > 0) {
        GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
    } else {
        if (obj == mp->itemObj[0])
            current_selected_item = mp->max_scroll;
        GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
    }
    obj = GUI_GetSelected();
    if (obj == mp->itemObj[0] || obj == mp->itemObj[1]) {
        current_selected_item = -1;
        GUI_SetRelativeOrigin(LEFT_VIEW_ID, 0, 0);
    } else {
        current_selected_item += direction;
        if (!GUI_IsObjectInsideCurrentView(LEFT_VIEW_ID, obj)) {
            // selected item is out of the view, scroll the view
            if (obj == mp->itemObj[FIRST_PAGE_ITEM_IDX])
                GUI_SetRelativeOrigin(LEFT_VIEW_ID, 0, 0);
            else if (obj == mp->itemObj[last_item]) {
                u8 pages = mp->max_scroll/mp->entries_per_page;
                if (mp->max_scroll%mp->entries_per_page != 0)
                    pages++;
                GUI_SetRelativeOrigin(LEFT_VIEW_ID, 0, (pages -1) * (ITEM_HEIGHT +1) * 4);
            }
            else
                GUI_ScrollLogicalView(LEFT_VIEW_ID, (ITEM_HEIGHT +1) *2*direction);
        }
    }
    GUI_SetScrollbar(mp->scroll_bar, current_selected_item >=0?current_selected_item :0);
}

static u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            GUI_RemoveAllObjects();  // Discard unsaved items and exit to upper page
            PAGE_MixerInit(mp->top_channel);
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) { // press enter to enter curves setup
            /* guiObject_t *obj = GUI_GetSelected();
            if (obj != mp->itemObj[1]) {
                GUI_SetSelected(mp->itemObj[1]); // quick jump to save button
                LCD_SetRelativeOrigin(0, 0);
                okcancel_cb(obj, (void *)((long)1));
            } else
                return 0; */
            return 0;
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            navigate_items(-1);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            navigate_items(1);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

static u8 _action_cb_switch(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            MIXPAGE_ChangeTemplate(1);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
