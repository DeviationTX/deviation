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
static u8 top_channel = 0;

static u8 action_cb(u32 button, u8 flags, void *data);

static void _show_title(int page)
{
    (void)page;
    mp->entries_per_page = 2;
    mp->max_scroll = Model.num_channels + NUM_VIRT_CHANNELS > mp->entries_per_page ?
            Model.num_channels + NUM_VIRT_CHANNELS - mp->entries_per_page : Model.num_channels + NUM_VIRT_CHANNELS;
    PAGE_SetActionCB(action_cb);
    mp->top_channel = top_channel;
}

static void _show_page()
{
    u8 h = 12;
    int i;
    GUI_RemoveAllObjects();
    PAGE_ShowHeader(_tr("Mixer"));

    struct Mixer *mix = MIXER_GetAllMixers();
    guiObject_t *obj;
    u8 space = ITEM_HEIGHT + 1;
    u8 row = space;
    u8 w1 = 50;
    u8 w2 = LCD_WIDTH - w1 - ARROW_WIDTH - 4;
    for (i = 0; row < space * 5; i++) {
        u8 idx;
        labelDesc.style = LABEL_LEFT;
        u8 ch = mp->top_channel + i;
        if (ch >= Model.num_channels)
            ch += (NUM_OUT_CHANNELS - Model.num_channels);
        if (ch < NUM_OUT_CHANNELS) {
            obj = GUI_CreateButtonPlateText(0, row, w1, h,&labelDesc, MIXPAGE_ChanNameProtoCB, 0,
                    limitselect_cb, (void *)((long)ch));
        }
        else {
            obj = GUI_CreateLabelBox(0, row, w1, h, &labelDesc,
                                   MIXPAGE_ChanNameProtoCB, NULL, (const void *)((long)ch));
        }
        mp->itemObj[i *2] = obj;

        labelDesc.style = LABEL_CENTER;
        mp->itemObj[i *2 +1] = GUI_CreateButtonPlateText(w1 + 2, row, w2, h , &labelDesc, template_name_cb, 0,
                templateselect_cb, (void *)((long)ch));

        for (idx = 0; idx < NUM_MIXERS; idx++)
            if (mix[idx].src && mix[idx].dest == ch)
                break;
        if (idx != NUM_MIXERS) {
            row += space;
            enum TemplateType template = MIXER_GetTemplate(ch);
            labelDesc.style = LABEL_LEFTCENTER;
            GUI_CreateLabelBox(0, row, w1, h, &labelDesc, show_source, NULL, &mix[idx].src);
            if (template == MIXERTEMPLATE_EXPO_DR) {
                if (mix[idx].src == mix[idx+1].src && mix[idx].dest == mix[idx+1].dest && mix[idx+1].sw) {
                    GUI_CreateLabelBox(w1 + 2, row, 35, h, &labelDesc, show_source, NULL, &mix[idx+1].sw);
                }
                if (mix[idx].src == mix[idx+2].src && mix[idx].dest == mix[idx+2].dest && mix[idx+2].sw) {
                    GUI_CreateLabelBox(w1 + 2 + 37, row, 30, h, &labelDesc, show_source, NULL, &mix[idx+2].sw);
                }
            }
        }
        row += space;
    }
    mp->entries_per_page = i;
    mp->max_scroll = Model.num_channels + NUM_VIRT_CHANNELS > mp->entries_per_page ?
            Model.num_channels + NUM_VIRT_CHANNELS - mp->entries_per_page : Model.num_channels + NUM_VIRT_CHANNELS;
    if (!GUI_IsSelectable(mp->itemObj[selectedIdx])) {
        selectedIdx++;
    }
    GUI_SetSelected(mp->itemObj[selectedIdx]);

    mp->scroll_bar = GUI_CreateScrollbar(LCD_WIDTH - ARROW_WIDTH, ITEM_HEIGHT, LCD_HEIGHT - ITEM_HEIGHT,
            Model.num_channels + NUM_VIRT_CHANNELS, NULL, NULL, NULL);
    u8 current_row = mp->top_channel + selectedIdx/2;
    if (current_row > 0)
        GUI_SetScrollbar(mp->scroll_bar, current_row);
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
    top_channel = mp->top_channel; // backup mp->top_channel, so that we can restore previous position when re-entering this page
    current_row = mp->top_channel + selectedIdx/2;
    GUI_SetScrollbar(mp->scroll_bar, current_row);
}

u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("Menu", PREVIOUS_ITEM);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            navigate_item(-1, 1);
        }else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            navigate_item(1, 1);
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            navigate_item(1, mp->entries_per_page);
        }  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
            navigate_item(-1, mp->entries_per_page);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

