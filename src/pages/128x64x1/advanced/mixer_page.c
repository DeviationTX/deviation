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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "../pages.h"
#include "config/model.h"
//#include "icons.h"

enum {
    COL1_X     = 0,
    COL1_W     = 35,
    COL2_X     = 36,
    COL2_W     = 46,
    COL3_X     = 85,
    COL3_W     = 45,
};
#endif //OVERRIDE_PLACEMENT

#include "../../common/advanced/_mixer_page.c"

static unsigned action_cb(u32 button, unsigned flags, void *data);
static int row_cb(int absrow, int relrow, int y, void *data);

static void _show_title()
{
    PAGE_SetActionCB(action_cb);
}

static void _show_page()
{
    PAGE_ShowHeader(_tr("Mixer    Reorder:Hold R+"));
    memset(gui, 0, sizeof(*gui));
    
    u8 channel_count = Model.num_channels + NUM_VIRT_CHANNELS;
    
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, channel_count, row_cb, NULL, NULL, NULL);// 7 lines
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    u8 idx;
    struct Mixer *mix = MIXER_GetAllMixers();

    int selectable = 2;
    int channel = absrow;
    if (channel >= Model.num_channels)
        channel += (NUM_OUT_CHANNELS - Model.num_channels);
    if (channel < NUM_OUT_CHANNELS) {
        labelDesc.align = ALIGN_LEFT;
        GUI_CreateButtonPlateText(&gui->limit[relrow], COL1_X, y, COL1_W, LINE_HEIGHT, &labelDesc, MIXPAGE_ChanNameProtoCB,
                limitselect_cb, (void *)((long)channel));
    } else if(! _is_virt_cyclic(channel)) {
        GUI_CreateButtonPlateText(&gui->limit[relrow], COL1_X, y, COL1_W, LINE_HEIGHT, &labelDesc, MIXPAGE_ChanNameProtoCB,
                virtname_cb, (void *)((long)channel));
    } else {
        GUI_CreateLabelBox(&gui->name[relrow], COL1_X, y, COL1_W, LINE_HEIGHT, &LABEL_FONT,
                MIXPAGE_ChanNameProtoCB, NULL, (const void *)((long)channel));
        selectable = 1;
    }
    labelDesc.align = ALIGN_CENTER;
    GUI_CreateButtonPlateText(&gui->tmpl[relrow], COL2_X, y, COL2_W, LINE_HEIGHT , &labelDesc, template_name_cb,
        templateselect_cb, (void *)((long)channel));
   
    for (idx = 0; idx < NUM_MIXERS; idx++)
        if (mix[idx].src && mix[idx].dest == channel)
            break;
    if (idx != NUM_MIXERS) {
        // don't show source if curve type is fixed, works only for the first mix per channel
        if(CURVE_TYPE(&mix[idx].curve) != CURVE_FIXED){
            GUI_CreateLabelBox(&gui->src[relrow], COL3_X, y, COL3_W , LINE_HEIGHT, &DEFAULT_FONT, show_source, NULL, &mix[idx].src);
        }
    }
    return selectable;
}

unsigned action_cb(u32 button, unsigned flags, void *data)
{
    if ((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
        reorder_cb(NULL, NULL);
        return 1;
    }
    return default_button_action_cb(button, flags, data);
}

