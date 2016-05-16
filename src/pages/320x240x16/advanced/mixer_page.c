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
#include "../icons.h"

#include "../../common/advanced/_mixer_page.c"

void PAGE_MixerExit()
{
}

static void _show_title()
{
    mp->max_scroll = Model.num_channels + NUM_VIRT_CHANNELS > ENTRIES_PER_PAGE ?
                          Model.num_channels + NUM_VIRT_CHANNELS - ENTRIES_PER_PAGE
                        : Model.num_channels + NUM_VIRT_CHANNELS;
    memset(gui, 0, sizeof(*gui));
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MIXER));
    GUI_CreateIcon(&gui->testico, LCD_WIDTH-128, 0, &icons[ICON_CHANTEST], show_chantest_cb, NULL);
    GUI_CreateIcon(&gui->reorderico, LCD_WIDTH-96, 0, &icons[ICON_ORDER], reorder_cb, NULL);
}

#undef XOFFSET
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    static const int XOFFSET = ((LCD_WIDTH - 320) / 2);

    struct Mixer *mix = MIXER_GetAllMixers();
    unsigned idx;
    unsigned ch = absrow;
    unsigned row = y;
    int selectable = 2;
    if (ch >= Model.num_channels)
        ch += (NUM_OUT_CHANNELS - Model.num_channels);
    if (ch < NUM_OUT_CHANNELS) {
        GUI_CreateButton(&gui->name[relrow].but, XOFFSET+4, row, BUTTON_64x16, MIXPAGE_ChanNameProtoCB,
                               0x0000, limitselect_cb, (void *)((long)ch));
    } else if(! _is_virt_cyclic(ch)) {
        GUI_CreateButton(&gui->name[relrow].but, XOFFSET+4, row, BUTTON_64x16, MIXPAGE_ChanNameProtoCB,
                               0x0000, virtname_cb, (void *)(long)ch);
    } else {
        GUI_CreateLabelBox(&gui->name[relrow].lbl, XOFFSET+4, row, 64, 18, &DEFAULT_FONT,
                               MIXPAGE_ChanNameProtoCB, NULL, (void *)((long)ch));
        selectable = 1;
    }
    GUI_CreateButton(&gui->tmpl[relrow], XOFFSET+132, row, BUTTON_64x16, template_name_cb, 0x0000,
                     templateselect_cb, (void *)((long)ch));
    for (idx = 0; idx < NUM_MIXERS; idx++)
        if (mix[idx].src && mix[idx].dest == ch)
            break;
    if (idx != NUM_MIXERS) {
        enum TemplateType template = MIXER_GetTemplate(ch);
        if (CURVE_TYPE(&mix[idx].curve) != CURVE_FIXED)
            GUI_CreateLabelBox(&gui->src[relrow], XOFFSET+68, row, 60, 18, &NARROW_FONT, show_source, NULL, &mix[idx].src);
        if (template == MIXERTEMPLATE_EXPO_DR) {
            if (mix[idx].src == mix[idx+1].src && mix[idx].dest == mix[idx+1].dest && mix[idx+1].sw) {
                GUI_CreateLabelBox(&gui->sw1[relrow], XOFFSET+200, row, 52, 18, &SMALL_FONT, show_source, NULL, &mix[idx+1].sw);
            }
            if (mix[idx].src == mix[idx+2].src && mix[idx].dest == mix[idx+2].dest && mix[idx+2].sw) {
                GUI_CreateLabelBox(&gui->sw2[relrow], XOFFSET+252, row, 52, 18, &SMALL_FONT, show_source, NULL, &mix[idx+2].sw);
            }
        }
    }
    return selectable;
}

static void _show_page()
{
    // Note for future maintenance: DO NOT use logical view to draw all the channel items at a time for this page:  I just
    // gave it a try, it spent very long time to construct all the 30 items and could trigger watch-dog to reboot !!!
    int init_y = LCD_HEIGHT == 240 ? 44 : 36;

    u8 channel_count = Model.num_channels + NUM_VIRT_CHANNELS;

    GUI_CreateScrollable(&gui->scrollable, 0, init_y, LCD_WIDTH, LCD_HEIGHT - init_y,
                     24, channel_count, row_cb, NULL, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

