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
#include "config/model.h"
//#include "icons.h"

static u16 current_selected = 0;
#include "../../common/advanced/_mixer_page.c"

static u8 top_channel = 0;

static unsigned action_cb(u32 button, unsigned flags, void *data);
static int row_cb(int absrow, int relrow, int y, void *data);
static guiObject_t *getobj_cb(int relrow, int col, void *data);

static void _show_title(int page)
{
    (void)page;
    PAGE_SetActionCB(action_cb);
    mp->top_channel = top_channel;
}

static void _show_page()
{
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(_tr("Mixer    Reorder:Hold R+"));
    memset(gui, 0, sizeof(*gui));
    
    u8 channel_count = Model.num_channels + NUM_VIRT_CHANNELS;
    
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, channel_count, row_cb, getobj_cb, NULL, NULL);// 7 lines
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, current_selected));
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    u8 w1 = 35;
    u8 w2 = 46;
    u8 w3 = 45;
    u8 idx;
    struct Mixer *mix = MIXER_GetAllMixers();

    int channel = mp->top_channel + absrow;
    if (channel >= Model.num_channels)
        channel += (NUM_OUT_CHANNELS - Model.num_channels);
    if (channel < NUM_OUT_CHANNELS) {
        labelDesc.style = LABEL_LEFT;
        GUI_CreateButtonPlateText(&gui->limit[relrow], 0, y, w1, LINE_HEIGHT, &labelDesc, MIXPAGE_ChanNameProtoCB, 0,
                limitselect_cb, (void *)((long)channel));
    } else if(! _is_virt_cyclic(absrow)) {
        GUI_CreateButtonPlateText(&gui->limit[relrow], 0, y, w1, LINE_HEIGHT, &labelDesc, MIXPAGE_ChanNameProtoCB, 0,
                virtname_cb, (void *)((long)channel));
    } else {
        GUI_CreateLabelBox(&gui->name[relrow], 0, y, w1, LINE_HEIGHT, &labelDesc,
                MIXPAGE_ChanNameProtoCB, NULL, (const void *)((long)channel));
    }
    labelDesc.style = LABEL_CENTER;
    GUI_CreateButtonPlateText(&gui->tmpl[relrow], w1 + 1, y, w2, LINE_HEIGHT , &labelDesc, template_name_cb, 0,
        templateselect_cb, (void *)((long)channel));
    
    for (idx = 0; idx < NUM_MIXERS; idx++)
        if (mix[idx].src && mix[idx].dest == channel)
            break;
    if (idx != NUM_MIXERS) {
        // don't show source if curve type is fixed, works only for the first mix per channel
        if(CURVE_TYPE(&mix[idx].curve) != CURVE_FIXED){
            labelDesc.style = LABEL_LEFT;
            GUI_CreateLabelBox(&gui->src[relrow], w1 + w2 + 4, y, w3 , LINE_HEIGHT, &labelDesc, show_source, NULL, &mix[idx].src);
        }
    }
    return 1;
}

void PAGE_MixerExit()
{
    current_selected = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    if(col==0)
        return (guiObject_t *)&gui->limit[relrow];
    return (guiObject_t *)&gui->tmpl[relrow];
}

unsigned action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        } else if ((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            reorder_cb(NULL, NULL);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

