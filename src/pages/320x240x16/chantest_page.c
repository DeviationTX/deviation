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

static void show_button_page();
static void _show_bar_page(u8 num_bars, u8 _page);

static s8 page;
static u8 num_pages;

static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)pos;
    (void)parent;
    (void)data;
    
    s8 newpos = page + (direction > 0 ? 1 : -1);
    if (newpos < 0)
        newpos = 0;
    else if (newpos >= num_pages)
        newpos = num_pages-1;
    if (newpos != page) {
        GUI_RemoveHierObjects((guiObject_t *)&gui->chan[0]);
        u8 count = (cp->type == MONITOR_CHANNELOUTPUT)
            ? Model.num_channels
            : NUM_INPUTS;
        _show_bar_page(count, newpos);
    }
    return page;
}

static void _show_bar_page(u8 num_bars, u8 _page)
{
    #define SEPERATION 32
    long i;
    u8 height;
    u8 count;
    int row_len;
    page = _page;
    num_pages = 0;

    if (num_bars > 18) {
        num_pages = (num_bars + 7) / 8 - 1;
    
        num_bars = num_bars - page * 8;
        if (num_bars > 16)
            num_bars = 16;
        row_len = 8;
    } else {
        row_len = 9;
    }

    cp->num_bars = num_bars;

    if (num_bars > row_len) {
        height = 70;
        count = (num_bars + 1) / 2;
    } else {
        height = 155;
        count = num_bars;
    }
    u16 offset = (320 + (SEPERATION - 10) - SEPERATION * ((num_pages > 1 ? 1 : 0) + count)) / 2;
    memset(cp->pctvalue, 0, sizeof(cp->pctvalue));
    for(i = 0; i < count; i++) {
        GUI_CreateLabelBox(&gui->chan[i], offset + SEPERATION * i - (SEPERATION - 10)/2, 32,
                                      SEPERATION, 19, &TINY_FONT, channum_cb, NULL, (void *)(i+8*page));
        GUI_CreateBarGraph(&gui->bar[i], offset + SEPERATION * i, 50, 10, height,
                                    -100, 100, BAR_VERTICAL,
                                    showchan_cb, (void *)i);
        GUI_CreateLabelBox(&gui->value[i], offset + SEPERATION * i - (SEPERATION - 10)/2, 53 + height,
                                      SEPERATION, 10, &TINY_FONT, value_cb, NULL, (void *)i);
    }
    offset = (320 + (SEPERATION - 10) - SEPERATION * ((num_pages > 1 ? 1 : 0) + (num_bars - count))) / 2;
    for(i = count; i < num_bars; i++) {
        GUI_CreateLabelBox(&gui->chan[i], offset + SEPERATION * (i - count) - (SEPERATION - 10)/2, 210 - height,
                                      SEPERATION, 19, &TINY_FONT, channum_cb, NULL, (void *)(i+8*page));
        GUI_CreateBarGraph(&gui->bar[i], offset + SEPERATION * (i - count), 229 - height, 10, height,
                                    -100, 100, BAR_VERTICAL,
                                    showchan_cb, (void *)i);
        GUI_CreateLabelBox(&gui->value[i], offset + SEPERATION * (i - count) - (SEPERATION - 10)/2, 230,
                                      SEPERATION, 10, &TINY_FONT, value_cb, NULL, (void *)i);
    }
    if(num_pages > 1) {
        GUI_CreateScrollbar(&gui->scrollbar, 304, 32, 208, num_pages, NULL, scroll_cb, NULL);
        GUI_SetScrollbar(&gui->scrollbar, page);
    }
        
}

void PAGE_ChantestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_CHANMON));
    cp->return_page = NULL;
    cp->type = MONITOR_CHANNELOUTPUT;
    _show_bar_page(Model.num_channels, 0);
}

void PAGE_InputtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_INPUTMON));
    cp->return_page = NULL;
    cp->type = MONITOR_RAWINPUT;
    _show_bar_page(NUM_INPUTS, 0);
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

    _show_bar_page(Model.num_channels, 0);
}

static void show_button_page()
{
    #define X_STEP 95
    int i;
    cp->is_locked = 3;
    int y = 64;
    GUI_CreateLabelBox(&gui->lock, 10, 40, 300, 20, &NARROW_FONT, lockstr_cb, NULL, NULL);
    for (i = 0; i < NUM_TX_BUTTONS; i++) {
        GUI_CreateLabelBox(&gui->chan[i], 10 + X_STEP * (i % 3), y, 0, 0,
                         &DEFAULT_FONT, button_str_cb, NULL, (void *)(long)i);
        GUI_CreateLabelBox(&gui->value[i], 70 + X_STEP * (i % 3), y, 16, 16,
                         &SMALLBOX_FONT, NULL, NULL, (void *)"");
        if ((i % 3) == 2)
            y += 24;
    }
}

void _handle_button_test()
{
    if (cp->is_locked == 0 && SPITouch_IRQ()) {
        BUTTON_RegisterCallback(&cp->action, 0xFFFFFFFF,
               BUTTON_PRESS | BUTTON_RELEASE | BUTTON_LONGPRESS | BUTTON_PRIORITY,
               button_capture_cb, NULL);
        GUI_Redraw(&gui->lock); //Textbox
        cp->is_locked++;
    } else if (cp->is_locked == 1 && ! SPITouch_IRQ()) {
        cp->is_locked++;
    } else if (cp->is_locked == 2 && SPITouch_IRQ()) {
        BUTTON_UnregisterCallback(&cp->action);
        GUI_Redraw(&gui->lock); //Textbox
        cp->is_locked++;
    } else if (cp->is_locked == 3 && ! SPITouch_IRQ()) {
        cp->is_locked = 0;
    }
    u32 buttons = ScanButtons();
    for (int i = 0; i < NUM_TX_BUTTONS; i++) {
        GUI_SetLabelDesc(&gui->value[i],
               CHAN_ButtonIsPressed(buttons, i+1)
               ? &SMALLBOXNEG_FONT
               : &SMALLBOX_FONT);
    }
    return;
}

static inline guiObject_t *_get_obj(int chan, int objid)
{
    return objid == ITEM_GRAPH ? (guiObject_t *)&gui->bar[chan] : (guiObject_t *)&gui->value[chan];
}
static inline int _get_input_idx(int chan)
{
    return page * 8 + chan;
}
