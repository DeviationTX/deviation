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

#ifndef OVERRIDE_PLACEMENT

#define RAW_FONT    DEFAULT_FONT.font
#define CHAN_FONT   TINY_FONT.font
#define RAW_HEIGHT  (LINE_HEIGHT + 5)
#define CHAN_HEIGHT 13
enum {
    LABEL_COL1_X = 1,
    LABEL_COL2_X = 64,
    LABEL_IDX_W  = 0,
    LABEL_CHAN_H = 8,
    CHAN_X_OFFSET = 40,
    LABEL_CHAN_W = 18,
    BAR_W        = 59,
    BAR_H        = 4,
    SCROLLABLE_X = 0,
    ARROW_W      = 10,
};

#endif //OVERRIDE_PLACEMENT
#include "../common/_chantest_page.c"

static unsigned _action_cb(u32 button, unsigned flags, void *data);
static const char *_page_cb(guiObject_t *obj, const void *data);
static const char *channum_cb(guiObject_t *obj, const void *data);

#ifndef DRAW_CHAN_OVERRRIDE
static void draw_chan(int disp, int row, int y)
{
    int x = disp%2 ? LABEL_COL2_X : LABEL_COL1_X;
    int idx = row * NUM_BARS_PER_ROW + (disp%2 ? 1 : 0);
    int height;
    struct LabelDesc labelValue = TINY_FONT;
    labelValue.align = ALIGN_RIGHT;
    struct LabelDesc labelSource = DEFAULT_FONT;
    if (cp->type == MONITOR_RAWINPUT) {
        labelSource.font = RAW_FONT;  // Could be translated to other languages, hence using 12normal
        height = LINE_HEIGHT;
    } else {
        labelSource.font = CHAN_FONT;  // only digits, can use smaller font to show more channels
        height = LABEL_CHAN_H;
    }
    GUI_CreateLabelBox(&gui->chan[idx], x, y,
        LABEL_IDX_W, height, &labelSource, channum_cb, NULL, (void *)(long)get_channel_idx(disp));
    GUI_CreateLabelBox(&gui->value[idx], x + CHAN_X_OFFSET, y,
        LABEL_CHAN_W, height, &labelValue, value_cb, NULL, (void *)(long)disp);
    if (BAR_H) {
        GUI_CreateBarGraph(&gui->bar[idx], x - 1, y + height,
            BAR_W, BAR_H, -125, 125, TRIM_HORIZONTAL, showchan_cb, (void *)(long)disp);
    }
}
#endif

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    switch(col) {
        case ITEM_GRAPH:  return BAR_H ? (guiObject_t *)&gui->bar[2*relrow] : (guiObject_t *)&gui->value[2*relrow];
        case ITEM_GRAPH2: return BAR_H ? (guiObject_t *)&gui->bar[2*relrow+1] :(guiObject_t *)&gui->value[2*relrow+1];
        case ITEM_VALUE2: return (guiObject_t *)&gui->value[2*relrow+1];
        default:          return (guiObject_t *)&gui->value[2*relrow];
    }
}
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    int idx = absrow * NUM_BARS_PER_ROW;
    draw_chan(idx, relrow, y);
    if(idx+1 < cp->num_bars)
        draw_chan(idx+1, relrow, y);
    return 0;
}

static void _show_bar_page(int row)
{
    cur_row = row;
    cp->num_bars = num_disp_bars();
    memset(cp->pctvalue, 0, sizeof(cp->pctvalue));
    int view_height = (cp->type == MONITOR_RAWINPUT)
                      ? RAW_HEIGHT   // can only show 3 rows: (12 + 5) x 3
                      : CHAN_HEIGHT;  // can only show 4 rows: (8 + 5) x 4
    GUI_CreateScrollable(&gui->scrollable, SCROLLABLE_X, HEADER_HEIGHT, LCD_WIDTH-SCROLLABLE_X, LCD_HEIGHT - HEADER_HEIGHT,
                         view_height, (cp->num_bars + 1)/2, row_cb, getobj_cb, NULL, NULL);
    GUI_CreateLabelBox(&gui->page, LCD_WIDTH -ARROW_W, 0, ARROW_W, HEADER_HEIGHT, &DEFAULT_FONT, _page_cb, NULL, NULL);
}

void PAGE_ChantestInit(int page)
{
    (void)page;
    PAGE_SetActionCB(_action_cb);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(cp->type == MONITOR_RAWINPUT ? _tr("Stick input") : _tr("Mixer output"));
    if (cp->type != MONITOR_RAWINPUT )
        cp->type = MONITOR_MIXEROUTPUT;// cp->type may not be initialized yet, so do it here
    _show_bar_page(0);
}

static void _navigate_pages(s8 direction)
{
    if ((direction == -1 && cp->type == MONITOR_RAWINPUT) ||
            (direction == 1 && cp->type == MONITOR_MIXEROUTPUT)) {
        cp->type = cp->type == MONITOR_RAWINPUT ? MONITOR_MIXEROUTPUT : MONITOR_RAWINPUT;
        PAGE_ChantestInit(0);
    }
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    if (CHAN_ButtonIsPressed(button, BUT_RIGHT) || CHAN_ButtonIsPressed(button, BUT_LEFT)) {
        if (flags & BUTTON_RELEASE) {
            _navigate_pages(CHAN_ButtonIsPressed(button, BUT_RIGHT) ? 1 : -1);
        }
        return 1;
    }
    return default_button_action_cb(button, flags, data);
}

static const char *channum_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    long ch = (long)data;
    INPUT_SourceName(tempstring, ch+1);
    return tempstring;
}

static const char *_page_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    tempstring_cpy((const char *)"->");  //this is actually used as an icon don't translate
    if (cp->type == MONITOR_RAWINPUT) {
        tempstring_cpy((const char *)"<-");
    }
    return tempstring;
}
void _handle_button_test() {}

static inline guiObject_t *_get_obj(int chan, int objid)
{
    return GUI_GetScrollableObj(&gui->scrollable, chan / 2, chan % 2 ? objid + 2 : objid);
}
