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
//#include "icons.h"

#include "../common/_mixer_page.c"

static u8 selectedIdx = 0;

static u8 action_cb(u32 button, u8 flags, void *data);

static void _show_title(int page)
{
    (void)page;
    mp->entries_per_page = 5;
    mp->max_scroll = Model.num_channels + NUM_VIRT_CHANNELS > mp->entries_per_page ?
            Model.num_channels + NUM_VIRT_CHANNELS - mp->entries_per_page : Model.num_channels + NUM_VIRT_CHANNELS;
    PAGE_SetActionCB(action_cb);
}

static void _show_page()
{
    int init_y = 0;
    u8 x = 0;
    u8 h = 12;
    int i;
    GUI_RemoveAllObjects();
    mp->labelDesc.font = DEFAULT_FONT.font;
    mp->labelDesc.font_color = 0xffff;
    mp->labelDesc.style = LABEL_LEFT;
    mp->labelDesc.outline_color = 0xffff;
    mp->labelDesc.fill_color = mp->labelDesc.outline_color;

    struct Mixer *mix = MIXER_GetAllMixers();
    guiObject_t *obj;
    for (i = 0; i < mp->entries_per_page; i++) {
        u8 idx;
        int row = init_y + (h +1) * i;
        u8 w = 40;
        mp->labelDesc.style = LABEL_LEFT;
        u8 ch = mp->top_channel + i;
        if (ch >= Model.num_channels)
            ch += (NUM_OUT_CHANNELS - Model.num_channels);
        if (ch < NUM_OUT_CHANNELS) {
            //obj = GUI_CreateLabelBox(x, row, w, h, &mp->labelDesc,
            //        MIXPAGE_ChanNameProtoCB, limitselect_cb, (const void *)((long)ch));
            //GUI_SetSelectable(obj, 1);
            obj = GUI_CreateButton(x, row, BUTTON_DEVO10, MIXPAGE_ChanNameProtoCB, 0,
                    limitselect_cb, (void *)((long)ch));
            GUI_CustomizeButton(obj, &mp->labelDesc, w, h);
        }
        else {
            obj = GUI_CreateLabelBox(x, row, w, h, &mp->labelDesc,
                                   MIXPAGE_ChanNameProtoCB, NULL, (const void *)((long)ch));
            GUI_SetSelectable(obj, 0);
        }
        mp->itemObj[i *2] = obj;

        w = 53;
        mp->labelDesc.style = LABEL_CENTER;
        //obj = GUI_CreateLabelBox(70, row, w, h, &mp->labelDesc, template_name_cb, templateselect_cb, (const void *)((long)ch));
        //GUI_SetSelectable(obj, 1);
        obj = GUI_CreateButton(69, row, BUTTON_DEVO10, template_name_cb, 0,
                templateselect_cb, (void *)((long)ch));
        GUI_CustomizeButton(obj, &mp->labelDesc, w, h);
        mp->itemObj[i *2 +1] = obj;

        w = 24;
        for (idx = 0; idx < NUM_MIXERS; idx++)
            if (mix[idx].src && mix[idx].dest == ch)
                break;

        if (idx != NUM_MIXERS) {
            //enum TemplateType template = MIXER_GetTemplate(ch);
            obj = GUI_CreateLabelBox(43, row, w, h, &mp->labelDesc, show_source, NULL, &mix[idx].src);
            GUI_SetSelectable(obj, 0);
        }
    }
    if (!GUI_IsSelectable(mp->itemObj[selectedIdx])) {
        selectedIdx++;
    }
    GUI_SetSelected(mp->itemObj[selectedIdx]);

    x = LCD_WIDTH - ARROW_WIDTH;
    mp->scroll_bar = GUI_CreateScrollbar(x, 0, LCD_HEIGHT, Model.num_channels + NUM_VIRT_CHANNELS, NULL, NULL, NULL);
}

static void _determine_save_in_live()
{
    if (mp->are_limits_changed) {
        mp->are_limits_changed = 0;
        MIXER_SetLimit(mp->channel, &mp->limit); // save limits' change in live
    }
}

void navigate_item(s8 direction, u8 step)
{
    struct guiObject *obj = GUI_GetSelected();
    u8 current_row;
    if (step == 1) { // scroll up or down 1 row
        if (direction > 0) {
            if (obj == mp->itemObj[2*mp->entries_per_page-1]) {
                if (mp->top_channel < mp->max_scroll ) {
                    mp->top_channel ++;
                    selectedIdx = 2*(mp->entries_per_page -1);
                    _show_page();
                }
            } else
                selectedIdx ++;
                if (!GUI_IsSelectable(mp->itemObj[selectedIdx])) {
                    selectedIdx++;
                }
                GUI_SetSelected(mp->itemObj[selectedIdx]);
        } else {
            struct guiObject *prevObj = GUI_GetPrevSelectable(obj);
            if (prevObj == mp->itemObj[2*mp->entries_per_page-1]) {  // the 1st item may not be selectable
                if (mp->top_channel > 0) {
                    mp->top_channel --;
                    selectedIdx = 1;
                    _show_page();
                }
            } else
                selectedIdx --;
                if (!GUI_IsSelectable(mp->itemObj[selectedIdx])) {
                    selectedIdx--;
                }
                GUI_SetSelected(mp->itemObj[selectedIdx]);
        }
    } else { // page up and page down
        current_row = selectedIdx/2;
        if (direction == 1) {
            s8 expected_row = mp->top_channel + current_row + step;
            if (expected_row > mp->max_scroll) {
                mp->top_channel = mp->max_scroll;
                selectedIdx = 2*mp->entries_per_page - 1;
            } else {
                mp->top_channel = expected_row;
                selectedIdx = 0;
            }
        } else {
            s8 expected_row = mp->top_channel - step;
            if (expected_row < 0) {
                mp->top_channel = 0;
                selectedIdx = 0;
            } else {
                mp->top_channel = expected_row;
                selectedIdx = 2*mp->entries_per_page - 1;
            }
        }
        _show_page();
    }
    current_row = mp->top_channel + selectedIdx/2;
    GUI_SetScrollbar(mp->scroll_bar, current_row);
}

u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("SubMenu", sub_menu_item);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            navigate_item(-1, 1);
        }else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            navigate_item(1, 1);
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            navigate_item(-1, mp->entries_per_page);
        }  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
            navigate_item(1, mp->entries_per_page);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

