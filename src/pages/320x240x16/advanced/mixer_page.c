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

#undef  ENTRIES_BY_SCREENSIZE
#undef  ENTRIES_PER_PAGE
#define ENTRIES_BY_SCREENSIZE (LCD_WIDTH == 480? 10 : 8) // 320x240: 8, 480x272: 10
#define ENTRIES_PER_PAGE      (ENTRIES_BY_SCREENSIZE > NUM_CHANNELS ? NUM_CHANNELS : ENTRIES_BY_SCREENSIZE)
static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data);

static void _show_title(int page)
{
    mp->max_scroll = Model.num_channels + NUM_VIRT_CHANNELS > ENTRIES_PER_PAGE ?
                          Model.num_channels + NUM_VIRT_CHANNELS - ENTRIES_PER_PAGE
                        : Model.num_channels + NUM_VIRT_CHANNELS;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_MIXER));
    GUI_CreateIcon(&gui->testico, LCD_WIDTH-128, 0, &icons[ICON_CHANTEST], show_chantest_cb, NULL);
    GUI_CreateIcon(&gui->reorderico, LCD_WIDTH-96, 0, &icons[ICON_ORDER], reorder_cb, NULL);
    GUI_CreateScrollbar(&gui->scroll, LCD_WIDTH-16, 32, LCD_HEIGHT-32, mp->max_scroll+1, NULL, scroll_cb, NULL);
    GUI_SetScrollbar(&gui->scroll, page);
}

static void _show_page()
{
    // Note for future maintenance: DO NOT use logical view to draw all the channel items at a time for this page:  I just
    // gave it a try, it spent very long time to construct all the 30 items and could trigger watch-dog to reboot !!!
    #define XOFFSET ((LCD_WIDTH - 320) / 2)
    int init_y = LCD_HEIGHT == 240 ? 44 : 36;
    int i;
    if (mp->firstObj) {
        GUI_RemoveHierObjects(mp->firstObj);
        mp->firstObj = NULL;
    }
    struct Mixer *mix = MIXER_GetAllMixers();
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        guiObject_t *obj;
        u8 idx;
        int row = init_y + 24 * i;
        u8 ch = mp->top_channel + i;
        if (ch >= Model.num_channels)
            ch += (NUM_OUT_CHANNELS - Model.num_channels);
        if (ch < NUM_OUT_CHANNELS) {
            obj = GUI_CreateButton(&gui->name[i].but, XOFFSET+4, row, BUTTON_64x16, MIXPAGE_ChanNameProtoCB,
                                   0x0000, limitselect_cb, (void *)((long)ch));
        } else if(! _is_virt_cyclic(ch)) {
            obj = GUI_CreateButton(&gui->name[i].but, XOFFSET+4, row, BUTTON_64x16, MIXPAGE_ChanNameProtoCB,
                                   0x0000, virtname_cb, (void *)(long)ch);
        } else {
            obj = GUI_CreateLabelBox(&gui->name[i].lbl, XOFFSET+4, row, 64, 16, &DEFAULT_FONT,
                                   MIXPAGE_ChanNameProtoCB, NULL, (void *)((long)ch));
        }
        if (! mp->firstObj)
            mp->firstObj = obj;
        GUI_CreateButton(&gui->tmpl[i], XOFFSET+132, row, BUTTON_64x16, template_name_cb, 0x0000,
                         templateselect_cb, (void *)((long)ch));
        for (idx = 0; idx < NUM_MIXERS; idx++)
            if (mix[idx].src && mix[idx].dest == ch)
                break;
        if (idx != NUM_MIXERS) {
            enum TemplateType template = MIXER_GetTemplate(ch);
            GUI_CreateLabelBox(&gui->src[i], XOFFSET+68, row, 60, 16, &NARROW_FONT, show_source, NULL, &mix[idx].src);
            if (template == MIXERTEMPLATE_EXPO_DR) {
                if (mix[idx].src == mix[idx+1].src && mix[idx].dest == mix[idx+1].dest && mix[idx+1].sw) {
                    GUI_CreateLabelBox(&gui->sw1[i], XOFFSET+200, row, 52, 16, &SMALL_FONT, show_source, NULL, &mix[idx+1].sw);
                }
                if (mix[idx].src == mix[idx+2].src && mix[idx].dest == mix[idx+2].dest && mix[idx+2].sw) {
                    GUI_CreateLabelBox(&gui->sw2[i], XOFFSET+252, row, 52, 16, &SMALL_FONT, show_source, NULL, &mix[idx+2].sw);
                }
            }
        }
    }
}
#undef XOFFSET

static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data) {
    (void)parent;
    (void)data;
    s16 newpos;
    if (direction > 0) {
        newpos = pos + (direction > 1 ? ENTRIES_PER_PAGE : 1);
        if (newpos > mp->max_scroll)
            newpos = mp->max_scroll;
    } else {
        newpos = pos - (direction < -1 ? ENTRIES_PER_PAGE : 1);
        if (newpos < 0)
            newpos = 0;
    }
    if (newpos != pos) {
        mp->top_channel = newpos;
        _show_page();
    }
    return newpos;
}

