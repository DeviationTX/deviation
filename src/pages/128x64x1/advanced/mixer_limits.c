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
#include "../pages.h"
#include <stdlib.h>

#include "../../common/advanced/_mixer_limits.c"

static u8 action_cb(u32 button, u8 flags, void *data);
static void revert_cb(guiObject_t *obj, const void *data);

#define LEFT_VIEW_ID 0
#define FIRST_PAGE_ITEM_IDX  1  // 0 is the button obj

static s8 current_selected_item;

static void _show_titlerow()
{
    (void)okcancel_cb;
    PAGE_SetActionCB(action_cb);
    mp->entries_per_page = 4;
    memset(mp->itemObj, 0, sizeof(mp->itemObj));

    labelDesc.style = LABEL_UNDERLINE;
    u8 w = 50;
    titleObj = GUI_CreateLabelBox(0, 0 , LCD_WIDTH - w, ITEM_HEIGHT, &labelDesc,
            MIXPAGE_ChanNameProtoCB, NULL, (void *)(long)mp->channel);
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[0] = GUI_CreateButtonPlateText(LCD_WIDTH - w, 0, w, ITEM_HEIGHT, &labelDesc, NULL, 0, revert_cb, (void *)_tr("Revert"));

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
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y) , w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Reverse:"));
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, toggle_reverse_cb, reverse_cb, (void *)((long)mp->channel));

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Failsafe:"));
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, toggle_failsafe_cb, set_failsafe_cb, NULL);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Safety:"));
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, sourceselect_cb, set_source_cb, &mp->limit.safetysw);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Safe Val:"));
    labelDesc.style = LABEL_CENTER;
    mp->safeValObj = mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_safeval_cb, NULL);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Min Limit:"));
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_limits_cb, &mp->limit.min);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Max Limit:"));
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_limits_cb, &mp->limit.max);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, scalestring_cb, NULL, (void *)1L);
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_limitsscale_cb, &mp->limit.servoscale);
    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, scalestring_cb, NULL, (void *)0);
    labelDesc.style = LABEL_CENTER;
    mp->negscaleObj = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_limitsscale_cb, &mp->limit.servoscale_neg);
    mp->max_scroll++;

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Subtrim:"));
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_trimstep_cb, &mp->limit.subtrim);

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(GUI_MapToLogicalView(LEFT_VIEW_ID, x), GUI_MapToLogicalView(LEFT_VIEW_ID, y), w, ITEM_HEIGHT,
            &labelDesc, NULL, NULL, _tr("Speed:"));
    labelDesc.style = LABEL_CENTER;
    mp->itemObj[mp->max_scroll++] = GUI_CreateTextSelectPlate(GUI_MapToLogicalView(LEFT_VIEW_ID, x1), GUI_MapToLogicalView(LEFT_VIEW_ID, y),
            w, ITEM_HEIGHT, &labelDesc, NULL, set_limits_cb, &mp->limit.speed);

    // The following items are not draw in the logical view;
    y = ITEM_HEIGHT;
    x = LCD_WIDTH - ARROW_WIDTH;
    u8 h = LCD_HEIGHT - y ;
    mp->max_scroll -= FIRST_PAGE_ITEM_IDX;
    mp->scroll_bar = GUI_CreateScrollbar(x, y, h, mp->max_scroll, NULL, NULL, NULL);
    GUI_SetSelected(mp->itemObj[1]);
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
