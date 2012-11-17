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
#include "gui/gui.h"
#include "config/model.h"

#include "../common/_chantest_page.c"

static void _show_bar_page(u8 num_bars)
{
    #define SEPERATION 32
    int i;
    u8 height;
    u8 count;
    if (num_bars > 18)
        num_bars = 18;
    cp->num_bars = num_bars;
    if (num_bars > 9) {
        height = 70;
        count = (num_bars) / 2;
    } else {
        height = 155;
        count = num_bars;
    }
    u16 offset = (320 + (SEPERATION - 10) - SEPERATION * count) / 2;
    memset(cp->pctvalue, 0, sizeof(cp->pctvalue));
    for(i = 0; i < count; i++) {
        GUI_CreateLabelBox(offset + SEPERATION * i - (SEPERATION - 10)/2, 32,
                                      SEPERATION, 19, &TINY_FONT, channum_cb, NULL, (void *)(long)i);
        cp->bar[i] = GUI_CreateBarGraph(offset + SEPERATION * i, 50, 10, height,
                                    -100, 100, BAR_VERTICAL,
                                    showchan_cb, (void *)((long)i));
        cp->value[i] = GUI_CreateLabelBox(offset + SEPERATION * i - (SEPERATION - 10)/2, 53 + height,
                                      SEPERATION, 10, &TINY_FONT, value_cb, NULL, (void *)((long)i));
    }
    offset = (320 + (SEPERATION - 10) - SEPERATION * (num_bars - count)) / 2;
    for(i = count; i < num_bars; i++) {
        GUI_CreateLabelBox(offset + SEPERATION * (i - count) - (SEPERATION - 10)/2, 210 - height,
                                      SEPERATION, 19, &TINY_FONT, channum_cb, NULL, (void *)((long)i));
        cp->bar[i] = GUI_CreateBarGraph(offset + SEPERATION * (i - count), 229 - height, 10, height,
                                    -100, 100, BAR_VERTICAL,
                                    showchan_cb, (void *)((long)i));
        cp->value[i] = GUI_CreateLabelBox(offset + SEPERATION * (i - count) - (SEPERATION - 10)/2, 230,
                                      SEPERATION, 10, &TINY_FONT, value_cb, NULL, (void *)((long)i));
    }
}

void PAGE_ChantestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_CHANMON));
    cp->return_page = NULL;
    cp->type = MONITOR_CHANNELOUTPUT;
    _show_bar_page(Model.num_channels);
}

void PAGE_InputtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_INPUTMON));
    cp->return_page = NULL;
    cp->type = MONITOR_RAWINPUT;
    _show_bar_page(NUM_INPUTS);
}

void PAGE_ButtontestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_BTNMON));
    cp->return_page = NULL;
    cp->type = MONITOR_BUTTONTEST;
    show_button_page();
}

void PAGE_ChantestModal(void(*return_page)(int page), int page)
{
    PAGE_SetModal(1);
    cp->return_page = return_page;
    cp->return_val = page;
    cp->type = MONITOR_CHANNELOUTPUT;
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_CHANMON), okcancel_cb);

    _show_bar_page(Model.num_channels);
}
