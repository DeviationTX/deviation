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
static const char *channum_cb(guiObject_t *obj, const void *data);

static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)pos;
    (void)parent;
    (void)data;
    
    int newpos = cur_row + (direction > 0 ? 1 : -1);
    int endpos = gui->scrollbar.num_items - 1;
    if (newpos < 0)
        newpos = 0;
    else if (newpos > endpos)
        newpos = endpos;
    if (newpos != cur_row) {
        GUI_RemoveHierObjects((guiObject_t *)&gui->chan[0]);
        FullRedraw = REDRAW_ONLY_DIRTY;
        GUI_DrawBackground(0, 32, LCD_WIDTH - 16, LCD_HEIGHT - 32);
        _show_bar_page(newpos);
    }
    return cur_row;
}

static void _show_bar_page(int row)
{
    cur_row = row;
    int i;
    u8 height;
    u8 count;
    int num_bars = num_disp_bars();
    int num_rows = 1;
    int row_len;

    if (num_bars > 2 * (NUM_BARS_PER_ROW + 1)) {
        num_rows = num_bars / NUM_BARS_PER_ROW + 1;
        num_bars -= cur_row * NUM_BARS_PER_ROW;
        if (num_bars > 2 * NUM_BARS_PER_ROW)
            num_bars = 2 * NUM_BARS_PER_ROW;
        row_len = NUM_BARS_PER_ROW;
    } else {
        row_len = NUM_BARS_PER_ROW + 1;
    }

    cp->num_bars = num_bars;

    if (num_bars > row_len) {
        height = 70 + (LCD_HEIGHT - 240) / 2;
        count = (num_bars + 1) / 2;
    } else {
        height = 155 + (LCD_HEIGHT - 240);
        count = num_bars;
    }
    u16 offset = (LCD_WIDTH + (SEPARATION - 10) - SEPARATION * ((num_rows > 2 ? 1 : 0) + count)) / 2;
    memset(cp->pctvalue, 0, sizeof(cp->pctvalue));
    for(i = 0; i < count; i++) {
        GUI_CreateLabelBox(&gui->chan[i], offset + SEPARATION * i - (SEPARATION - 10)/2, 32,
                                      SEPARATION, 19, &TINY_FONT, channum_cb, NULL, (void *)(long)i);
        GUI_CreateBarGraph(&gui->bar[i], offset + SEPARATION * i, 52, 10, height,
                                    -100, 100, BAR_VERTICAL,
                                    showchan_cb, (void *)(long)i);
        GUI_CreateLabelBox(&gui->value[i], offset + SEPARATION * i - (SEPARATION - 10)/2, 53 + height,
                                      SEPARATION, 10, &TINY_FONT, value_cb, NULL, (void *)(long)i);
    }
    offset = (LCD_WIDTH + (SEPARATION - 10) - SEPARATION * ((num_rows > 2 ? 1 : 0) + (num_bars - count))) / 2;
    for(i = count; i < num_bars; i++) {
        GUI_CreateLabelBox(&gui->chan[i], offset + SEPARATION * (i - count) - (SEPARATION - 10)/2, 210 + (LCD_HEIGHT - 240) - height,
                                      SEPARATION, 19, &TINY_FONT, channum_cb, NULL, (void *)(long)i);
        GUI_CreateBarGraph(&gui->bar[i], offset + SEPARATION * (i - count), 230 + (LCD_HEIGHT - 240) - height, 10, height,
                                    -100, 100, BAR_VERTICAL,
                                    showchan_cb, (void *)(long)i);
        GUI_CreateLabelBox(&gui->value[i], offset + SEPARATION * (i - count) - (SEPARATION - 10)/2, 230 + (LCD_HEIGHT - 240),
                                      SEPARATION, 10, &TINY_FONT, value_cb, NULL, (void *)(long)i);
    }
    if(num_rows > 2) {
        GUI_CreateScrollbar(&gui->scrollbar, LCD_WIDTH-16, 32, LCD_HEIGHT-32, num_rows-1, NULL, scroll_cb, NULL);
        GUI_SetScrollbar(&gui->scrollbar, cur_row);
    }
}

void PAGE_ChantestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_CHANMON));
    cp->return_page = NULL;
    cp->type = MONITOR_MIXEROUTPUT;
    _show_bar_page(0);
}

void PAGE_InputtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_INPUTMON));
    cp->return_page = NULL;
    cp->type = MONITOR_RAWINPUT;
    _show_bar_page(0);
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
    cp->type = MONITOR_MIXEROUTPUT;
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_CHANMON), okcancel_cb);

    _show_bar_page(0);
}

static void show_button_page()
{
    // show elements where they are located on the real tx
    enum {
        OFFSET_X    = ((LCD_WIDTH - 320) / 2), // center on Devo12-screen
        OFFSET_Y    = ((LCD_HEIGHT - 240) / 2),
    };
    enum {X = 0, Y = 1};
    struct LabelDesc alignRight = {
        .font = DEFAULT_FONT.font,
        .style = LABEL_RIGHT,
        .font_color = DEFAULT_FONT.font_color,
        .fill_color = DEFAULT_FONT.fill_color,
        .outline_color = DEFAULT_FONT.outline_color
    };

    const int label_pos[NUM_TX_BUTTONS][2] = CHANTEST_BUTTON_PLACEMENT;
    cp->is_locked = 3;
    GUI_CreateLabelBox(&gui->lock, OFFSET_X, 34, 320, 20, &NARROW_FONT, lockstr_cb, NULL, NULL);
    for (int i = 0; i < NUM_TX_BUTTONS; i++) {
        if ((1 << (i + 1)) & Transmitter.ignore_buttons)
            continue;
        GUI_CreateLabelBox(&gui->value[i],
                OFFSET_X + (label_pos[i][X] > 0 ? label_pos[i][X] + 50 : -label_pos[i][X] -20),    // >0? box at left side of label, otherwise right
                OFFSET_Y + label_pos[i][Y],
                16, 16,
                &SMALLBOX_FONT, NULL, NULL, (void *)"");
        GUI_CreateLabelBox(&gui->chan[i],
                OFFSET_X + abs(label_pos[i][X]),                                         // no differencing for the label
                OFFSET_Y + label_pos[i][Y],
                48, 16,
                label_pos[i][X] > 0 ? &alignRight : &DEFAULT_FONT,
                button_str_cb, NULL, (void *)(long)i);
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

static const char *channum_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int disp = (long)data;
    int ch = get_channel_idx(cur_row * NUM_BARS_PER_ROW + disp);
    if (cp->type) {
        char *p = tempstring;
        if (disp & 0x01) {
            *p = '\n';
            p++;
        }
        CONFIG_EnableLanguage(0);  //Disable translation because tiny font is limited in character set
        INPUT_SourceName(p, ch+1);
        CONFIG_EnableLanguage(1);
        if (! (disp & 0x01)) {
            sprintf(p + strlen(p), "\n");
        }
    } else {
        ch -= NUM_INPUTS;
        if (ch < NUM_OUT_CHANNELS) {
            sprintf(tempstring, "\n%d", ch+1);
        } else {
            ch -= NUM_OUT_CHANNELS;
            if (Model.virtname[ch][0]) {
                tempstring_cpy(Model.virtname[ch]) ;
            } else {
                sprintf(tempstring, "%s%d", _tr("Virt"), ch+1);
            }
        }
    }
    return tempstring;
}
