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
static u8 expo1_start_id;
static u8 expo2_start_id;
static u8 current_xygraph = 0;

static u8 action_cb(u32 button, u8 flags, void *data);

static void _show_titlerow()
{
    PAGE_SetActionCB(action_cb);
    mp->entries_per_page = 2;
    memset(mp->itemObj, 0, sizeof(mp->itemObj));

    labelDesc.style = LABEL_UNDERLINE;
    labelDesc.font_color = labelDesc.fill_color = labelDesc.outline_color = 0xffff;
    GUI_CreateLabelBox(0, 0 , LCD_WIDTH, ITEM_HEIGHT, &labelDesc,
            MIXPAGE_ChanNameProtoCB, NULL, (void *)((long)mp->cur_mixer->dest));
    u8 x =40;
    u8 w = 50;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[0] = GUI_CreateTextSelectPlate(x, 0,  w, ITEM_HEIGHT, &labelDesc, NULL, templatetype_cb, (void *)((long)mp->channel));
    GUI_SetSelected(mp->itemObj[0]);
    w = 38;
    mp->itemObj[1] = GUI_CreateButtonPlateText(LCD_WIDTH - w, 0, w, ITEM_HEIGHT, &labelDesc, NULL, 0, okcancel_cb, (void *)_tr("Save"));

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
    labelDesc.style = LABEL_LEFTCENTER;
    mp->firstObj = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Src:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, sourceselect_cb,
            set_source_cb, &mp->mixer[0].src);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Curve:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, curveselect_cb, set_curvename_cb, &mp->mixer[0]);

    y += space;  // out of current logical view, won't show by default
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL,_tr("Scale:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL,
            set_number100_cb, &mp->mixer[0].scalar);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Offset:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL,
            set_number100_cb, &mp->mixer[0].offset);

    // The following items are not draw in the logical view;
    mp->graphs[0] = GUI_CreateXYGraph(77, LCD_HEIGHT - RIGHT_VIEW_HEIGHT - 1, RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb,
                              &mp->mixer[0]);
    mp->graphs[1] = mp->graphs[2] = NULL;

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
    labelDesc.style = LABEL_LEFTCENTER;
    //Row 1
    if (! mp->expoObj[0]) {
        mp->firstObj = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
                &labelDesc, NULL, NULL, _tr("Mixers:"));
        y += space;
        labelDesc.style = LABEL_CENTER;
        mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &labelDesc, NULL, set_nummixers_cb, NULL);

        y += space;
        labelDesc.style = LABEL_LEFTCENTER;
        GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
                        &labelDesc, NULL, NULL, _tr("Page:"));
        y += space;
        labelDesc.style = LABEL_CENTER;
        mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &labelDesc, reorder_cb, set_mixernum_cb, NULL);
    } else {
        GUI_RemoveHierObjects(mp->expoObj[0]);
        y += 3*space;
    }

    //Row 2
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    mp->expoObj[0] = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Switch:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, sourceselect_cb, set_source_cb, &mp->cur_mixer->sw);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Mux:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_mux_cb, NULL);


    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Src:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, sourceselect_cb, set_source_cb, &mp->cur_mixer->src);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Curve:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, curveselect_cb, set_curvename_cb, mp->cur_mixer);


    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Scale:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_number100_cb, &mp->cur_mixer->scalar);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Offset:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_number100_cb, &mp->cur_mixer->offset);

    y += space;
    mp->trimObj = GUI_CreateButtonPlateText(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, show_trim_cb, 0x0000, toggle_trim_cb, NULL);
    mp->itemObj[mp->max_scroll++] = mp->trimObj;
    if (! MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
        GUI_SetHidden(mp->trimObj, 1);


    // The following items are not draw in the logical view;
    mp->bar = GUI_CreateBarGraph(LEFT_VIEW_WIDTH +10, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1, 5, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL, eval_chan_cb, NULL);
    mp->graphs[0] = GUI_CreateXYGraph(77, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1, RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                                  CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                                  CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                                  0, 0, eval_mixer_cb, curpos_cb, touch_cb, mp->cur_mixer);
    mp->graphs[2] = NULL;

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
    labelDesc.style = LABEL_LEFTCENTER;
    mp->firstObj = GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Src:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, sourceselect_cb, set_source_cb, &mp->mixer[0].src);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("High-Rate"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc,  curveselect_cb, set_curvename_cb, &mp->mixer[0]);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Scale:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc,  NULL, set_number100_cb, &mp->mixer[0].scalar);

    // 2nd template
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Switch1"));
    GUI_CreateRect(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , LEFT_VIEW_WIDTH, 1, &labelDesc);
    y += space;
    labelDesc.style = LABEL_CENTER;
    expo1_start_id = mp->max_scroll;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &labelDesc,  sourceselect_cb, set_drsource_cb, &mp->mixer[1].sw);
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    mp->itemObj[mp->max_scroll++] = mp->expoObj[0] = GUI_CreateButtonPlateText(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) ,
            w, ITEM_HEIGHT, &labelDesc, show_rate_cb, 0xffff, toggle_link_cb, (void *)0);
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = mp->expoObj[2] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, curveselect_cb, set_curvename_cb, &mp->mixer[1]);
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Scale1:"));
    y += space;
    mp->itemObj[mp->max_scroll++] = mp->expoObj[3] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc,  NULL, set_number100_cb, &mp->mixer[1].scalar);

    // 3rd template
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Switch2"));
    GUI_CreateRect(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , LEFT_VIEW_WIDTH, 1, &labelDesc);
    y += space;
    labelDesc.style = LABEL_CENTER;
    expo2_start_id = mp->max_scroll;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
                w, ITEM_HEIGHT, &labelDesc,  sourceselect_cb, set_drsource_cb, &mp->mixer[2].sw);
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    mp->itemObj[mp->max_scroll++] = mp->expoObj[4] = GUI_CreateButtonPlateText(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) ,
            w, ITEM_HEIGHT, &labelDesc, show_rate_cb, 0xffff, toggle_link_cb, (void *)1);
    y += space;
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = mp->expoObj[6] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, curveselect_cb, set_curvename_cb, &mp->mixer[2]);
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Scale2:"));
    y += space;
    mp->itemObj[mp->max_scroll++] = mp->expoObj[7] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc,  NULL, set_number100_cb, &mp->mixer[2].scalar);

    GUI_SetupLogicalView(RIGHT_VIEW_ID, 0, 0, RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT, 77, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1);
    // The following items are draw in the right logical view,
    mp->bar = GUI_CreateBarGraph(LEFT_VIEW_WIDTH +10, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1, 5, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL, eval_chan_cb, NULL);
    // the right view will be scroll up/down by changed the switch 1/2 options
    y = 0;
    mp->graphs[0] = GUI_CreateXYGraph(GUI_MapToLogicalView(RIGHT_VIEW_ID, 0) ,
                            GUI_MapToLogicalView(RIGHT_VIEW_ID, y), RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, NULL, &mp->mixer[0]);
    y += RIGHT_VIEW_HEIGHT;
    mp->graphs[1] = GUI_CreateXYGraph(GUI_MapToLogicalView(RIGHT_VIEW_ID, 0) ,
                            GUI_MapToLogicalView(RIGHT_VIEW_ID, y), RIGHT_VIEW_HEIGHT, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, NULL, &mp->mixer[1]);
    y += RIGHT_VIEW_HEIGHT;
    mp->graphs[2] = GUI_CreateXYGraph(GUI_MapToLogicalView(RIGHT_VIEW_ID, 0) ,
                            GUI_MapToLogicalView(RIGHT_VIEW_ID, y), RIGHT_VIEW_HEIGHT , RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, NULL, &mp->mixer[2]);
    current_xygraph = 0; // bug fix: this flag is used to reduce screen refresh

    // The following items are not draw in the logical view;
    y = ITEM_HEIGHT;
    x = LEFT_VIEW_WIDTH + ARROW_WIDTH;
    u8 h = LCD_HEIGHT - y ;
    mp->max_scroll -= FIRST_PAGE_ITEM_IDX;
    mp->scroll_bar = GUI_CreateScrollbar(x, y, h, mp->max_scroll, NULL, NULL, NULL);

    //Enable/Disable the relevant widgets
    _update_rate_widgets(0);
    _update_rate_widgets(1);
}

static void _update_rate_widgets(u8 idx)
{
    u8 mix = idx + 1;
    idx *=4;
    if (MIXER_SRC(mp->mixer[mix].sw)) {
        GUI_ButtonEnable(mp->expoObj[idx], 1);
        if(mp->link_curves & mix ) {
            GUI_TextSelectEnable(mp->expoObj[idx +2], 0);
        } else {
            GUI_TextSelectEnable(mp->expoObj[idx +2], 1);
        }
        GUI_TextSelectEnable(mp->expoObj[idx +3], 1);
    } else {
        GUI_ButtonEnable(mp->expoObj[idx], 0);
        GUI_TextSelectEnable(mp->expoObj[idx +2], 0);
        GUI_TextSelectEnable(mp->expoObj[idx +3], 0);
    }
}

static void navigate_items(s8 direction)
{
    guiObject_t *obj = GUI_GetSelected();
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
        // Perf improvement on UI drawing for mixer setup: remove unnecessary view refreshing
        if (obj == mp->itemObj[0] &&direction > 0)
            GUI_SetRelativeOrigin(LEFT_VIEW_ID, 0, 0);
    } else {
        current_selected_item += direction;
        if (!GUI_IsObjectInsideCurrentView(LEFT_VIEW_ID, obj)) {
            // selected item is out of the view, scroll the view
            GUI_ScrollLogicalViewToObject(LEFT_VIEW_ID, obj, direction);
        }
    }
    if (mp->graphs[1] != NULL && mp->graphs[2] != NULL) { // indicate that it is the expo&dr page
        // Bug description: (only happen in real devo10) When config Expo&DR template, it is most likely that hitting the Save button will fail
        // it is because in previous design, whenever switching from one widget to another, the xygraph will be redrew and cause
        // timing issue to interfere with button evenst. To fix the issue, reducing screen-redraw is the key
        // the flag of current_xygraph is used to avoid unnecessary screen refresh and can fix this bug
        if (current_selected_item == -1 || current_selected_item < expo1_start_id -FIRST_PAGE_ITEM_IDX) {
            if (current_xygraph != 0) {
                current_xygraph = 0;
                GUI_SetRelativeOrigin(RIGHT_VIEW_ID, 0, 0);
            }
        }
        else if (current_selected_item >= expo2_start_id -FIRST_PAGE_ITEM_IDX) {
            if (current_xygraph != 2) {
                current_xygraph = 2;
                GUI_SetRelativeOrigin(RIGHT_VIEW_ID, 0, RIGHT_VIEW_HEIGHT + RIGHT_VIEW_HEIGHT);
            }
        }
        else if (current_xygraph != 1) {
            GUI_SetRelativeOrigin(RIGHT_VIEW_ID, 0, RIGHT_VIEW_HEIGHT);
            current_xygraph = 1;
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
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)&& (flags & BUTTON_LONGPRESS)) {
            // long press enter = save without exiting
            PAGE_SaveMixerSetup(mp);
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
