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
#include <stdlib.h>

#include "../common/_mixer_limits.c"

static u8 action_cb(u32 button, u8 flags, void *data);
static void revert_cb(guiObject_t *obj, const void *data);
static void _toggle_reverse_cb_in_live(guiObject_t *obj, void *data);
static const char *_reverse_cb_in_live(guiObject_t *obj, int dir, void *data);

#define LEFT_VIEW_ID 0
#define FIRST_PAGE_ITEM_IDX  1  // 0 is the button obj
#define LONG_PRESS_STEP 10  // for devo10, would prefer 10 instead

static s8 current_selected_item;
static guiObject_t *titleObj = NULL;
static struct Limit origin_limit;

static void _show_titlerow()
{
    (void)okcancel_cb;
    mp->are_limits_changed = 0;
    PAGE_SetActionCB(action_cb);
    mp->entries_per_page = 4;
    memset(mp->itemObj, 0, sizeof(mp->itemObj));
    memcpy(&origin_limit, (const void *)&mp->limit, sizeof(origin_limit)); // back up for reverting purpose

    mp->labelDesc.style = LABEL_UNDERLINE;
    titleObj = GUI_CreateLabelBox(0, 0 , LCD_WIDTH, ITEM_HEIGHT, &mp->labelDesc,
            MIXPAGE_ChanNameProtoCB, NULL, (void *)(long)mp->channel);
    u8 w = 50;
    mp->itemObj[0] = GUI_CreateButton(LCD_WIDTH - w, 0, BUTTON_DEVO10, NULL, 0, revert_cb, (void *)_tr("Revert"));
    GUI_CustomizeButton(mp->itemObj[0] , &mp->labelDesc, w, ITEM_HEIGHT);

    // Create a logical view
    u8 view_origin_absoluteX = 0;
    u8 view_origin_absoluteY = ITEM_HEIGHT + 1;
    u8 h = LCD_HEIGHT - view_origin_absoluteY ;
    GUI_SetupLogicalView(LEFT_VIEW_ID, 0, 0, LCD_WIDTH -5, h, view_origin_absoluteX, view_origin_absoluteY);
}

static void _show_limits()
{
    mp->max_scroll = FIRST_PAGE_ITEM_IDX;
    current_selected_item = 0;

    u8 x = 0;
    u8 space = ITEM_HEIGHT + 1;
    u8 w = 60;
    u8 y = 0;
    u8 x1 = 60;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Reverse:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, _toggle_reverse_cb_in_live, _reverse_cb_in_live, (void *)((long)mp->channel));

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Failsafe:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, toggle_failsafe_cb, set_failsafe_cb, NULL);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Safety:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, sourceselect_cb, set_source_cb, &mp->limit.safetysw);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Safe Val:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, PAGEMIXER_SetNumberCB, &mp->limit.safetyval);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Min:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_limits_cb, &mp->limit.min);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Max:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_limits_cb, &mp->limit.max);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Scale:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_limits_cb, &mp->limit.servoscale);

    y += space;
    mp->labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &mp->labelDesc, NULL, NULL, _tr("Subtrim:"));
    mp->labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &mp->labelDesc, NULL, set_trimstep_cb, &mp->limit.subtrim);

    // The following items are not draw in the logical view;
    y = space;
    x = LCD_WIDTH - 3;
    u8 h = LCD_HEIGHT - y ;
    mp->max_scroll -= FIRST_PAGE_ITEM_IDX;
    mp->scroll_bar = GUI_CreateScrollbar(x, y, h, mp->max_scroll, NULL, NULL, NULL);
    GUI_SetSelected(mp->itemObj[1]);
}

void _toggle_reverse_cb_in_live(guiObject_t *obj, void *data) {
    toggle_reverse_cb(obj, data);
    GUI_Redraw(titleObj);  // since changes are saved in live ,we need to redraw the title
    GUI_Redraw(mp->itemObj[0]);
}

const char *_reverse_cb_in_live(guiObject_t *obj, int dir, void *data)
{
    const char *ret_str = reverse_cb(obj, dir, data);
    GUI_Redraw(titleObj);  // since changes are saved in live ,we need to redraw the title
    GUI_Redraw(mp->itemObj[0]);
    return ret_str;
}

void revert_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    memcpy(&mp->limit, (const void *)&origin_limit, sizeof(origin_limit));
    MIXER_SetLimit(mp->channel, &mp->limit);  // save
    GUI_DrawScreen();
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
    if (obj == mp->itemObj[0]) {
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
                GUI_ScrollLogicalView(LEFT_VIEW_ID, (ITEM_HEIGHT +1) *direction);
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
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            navigate_items(-1);
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            navigate_items(1);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
