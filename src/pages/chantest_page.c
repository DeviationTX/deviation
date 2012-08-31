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

static struct chantest_page * const cp = &pagemem.u.chantest_page;

static s16 showchan_cb(void *data);
static const char *value_cb(guiObject_t *obj, const void *data);
static const char *channum_cb(guiObject_t *obj, const void *data);

static void show_bar_page(u8 num_bars)
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

const char *lockstr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if(cp->is_locked == 1 || cp->is_locked == 2)
        return _tr("Touch to Unlock");
    else
        return _tr("Touch to Lock");
}

const char *button_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int button = (long)data;
    return INPUT_ButtonName(button + 1);
}

static void show_button_page()
{
    #define X_STEP 95
    int i;
    cp->is_locked = 3;
    int y = 64;
    cp->bar[0] = GUI_CreateLabelBox(100, 40, 0, 0, &DEFAULT_FONT, lockstr_cb, NULL, NULL);
    for (i = 0; i < NUM_TX_BUTTONS; i++) {
        GUI_CreateLabelBox(10 + X_STEP * (i % 3), y, 0, 0,
                         &DEFAULT_FONT, button_str_cb, NULL, (void *)(long)i);
        cp->value[i] = GUI_CreateLabelBox(70 + X_STEP * (i % 3), y, 16, 16,
                         &SMALLBOX_FONT, NULL, NULL, (void *)"");
        if ((i % 3) == 2)
            y += 24;
    }
}

void PAGE_ChantestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Channels"));
    cp->return_page = NULL;
    cp->type = 0;
    show_bar_page(Model.num_channels);
}

void PAGE_InputtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Inputs"));
    cp->return_page = NULL;
    cp->type = 1;
    show_bar_page(NUM_INPUTS);
}

void PAGE_ButtontestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(_tr("Buttons"));
    cp->return_page = NULL;
    cp->type = 2;
    show_button_page();
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if(cp->return_page) {
        PAGE_SetModal(0);
        PAGE_RemoveAllObjects();
        cp->return_page(1);
    }
}

void PAGE_ChantestModal(void(*return_page)(int page))
{
    PAGE_SetModal(1);
    cp->return_page = return_page;
    cp->type = 0;
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader_ExitOnly(_tr("Channels"), okcancel_cb);

    show_bar_page(Model.num_channels);
}
u8 button_capture_cb(u32 button, u8 flags, void *data)
{
    (void)button;
    (void)flags;
    (void)data;
    return 1;
}

void PAGE_ChantestEvent()
{
    int i;
    if(cp->type ==2) {
        if (cp->is_locked == 0 && SPITouch_IRQ()) {
            BUTTON_RegisterCallback(&cp->action, 0xFFFFFFFF,
                   BUTTON_PRESS | BUTTON_RELEASE | BUTTON_LONGPRESS | BUTTON_PRIORITY,
                   button_capture_cb, NULL);
            GUI_Redraw(cp->bar[0]); //Textbox
            cp->is_locked++;
        } else if (cp->is_locked == 1 && ! SPITouch_IRQ()) {
            cp->is_locked++;
        } else if (cp->is_locked == 2 && SPITouch_IRQ()) {
            BUTTON_UnregisterCallback(&cp->action);
            GUI_Redraw(cp->bar[0]); //Textbox
            cp->is_locked++;
        } else if (cp->is_locked == 3 && ! SPITouch_IRQ()) {
            cp->is_locked = 0;
        }
        u32 buttons = ScanButtons();
        for (i = 0; i < NUM_TX_BUTTONS; i++) {
            GUI_SetLabelDesc(cp->value[i],
                   CHAN_ButtonIsPressed(buttons, i+1)
                   ? &SMALLBOXNEG_FONT
                   : &SMALLBOX_FONT);
        }
        return;
    }
    s16 *raw = MIXER_GetInputs();
    for(i = 0; i < cp->num_bars; i++) {
        s8 v = RANGE_TO_PCT(cp->type ? raw[i+1] : Channels[i]);
        if (v != cp->pctvalue[i]) {
            GUI_Redraw(cp->bar[i]);
            GUI_Redraw(cp->value[i]);
            cp->pctvalue[i] = v;
        }
    }
}

void PAGE_ChantestExit()
{
    BUTTON_UnregisterCallback(&cp->action);
}
static s16 showchan_cb(void *data)
{
    long ch = (long)data;
    return cp->pctvalue[ch];
}

static const char *value_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    sprintf(cp->tmpstr, "%d", cp->pctvalue[ch]);
    return cp->tmpstr;
}

static const char *channum_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    if (cp->type) {
        char *p = cp->tmpstr;
        if (ch & 0x01) {
            *p = '\n';
            p++;
        }
        INPUT_SourceName(p, ch+1);
        if (! (ch & 0x01)) {
            sprintf(p + strlen(p), "\n");
        }
    } else {
       sprintf(cp->tmpstr, "\n%d", (int)ch+1);
    }
    return cp->tmpstr;
}
