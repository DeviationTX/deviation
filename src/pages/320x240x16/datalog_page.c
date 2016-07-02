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

#if HAS_DATALOG
#include "../common/_datalog_page.c"


static const int ROW1 = (DLOG_CHANNELS);
static const int ROW2 = (DLOG_LAST - DLOG_CHANNELS);
enum {
    SCROLLABLE_WIDTH = (275 + 16 + 5),
    SCROLLABLE_X     = (LCD_WIDTH - SCROLLABLE_WIDTH) / 2,
};

const char *emptystr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return "";
}

void press_cb(guiObject_t *obj, s8 press, const void *data)
{
    //press = -1 : release
    //press = 0  : short press
    //press = 1  : long press
    int absrow = (long)data;
    if (MIXER_SourceAsBoolean(Model.datalog.enable))
        return;
    if (press == 0) {
        dlog->source[DATALOG_BYTE(absrow)] ^= (1 << DATALOG_POS(absrow));
        GUI_SetLabelDesc((guiLabel_t *)obj,
               (dlog->source[DATALOG_BYTE(absrow)] & (1 << DATALOG_POS(absrow)))
               ? &SMALLBOXNEG_FONT
               : &SMALLBOX_FONT);
        DATALOG_UpdateState();
    }
}

static const char *remaining_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    snprintf(tempstring, sizeof(tempstring), _tr("%d bytes left"), DATALOG_Remaining());
    return tempstring;
}

static void select_press_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    (void)data;
    if (MIXER_SourceAsBoolean(Model.datalog.enable))
        return;
    memset(&dlog->source, data ? 0 : 0xff, sizeof(dlog->source));
    DATALOG_UpdateState();
    for(unsigned i = 0; i < sizeof(gui->checked) / sizeof(guiLabel_t); i++) {
        if(OBJ_IS_USED(&gui->checked[i]))
            GUI_SetLabelDesc(&gui->checked[i],
               data == 0
               ? &SMALLBOXNEG_FONT
               : &SMALLBOX_FONT);
        if(OBJ_IS_USED(&gui->checked2[i]))
            GUI_SetLabelDesc(&gui->checked2[i],
               data == 0
               ? &SMALLBOXNEG_FONT
               : &SMALLBOX_FONT);
    }
    //GUI_Redraw((guiObject_t *)&gui->scrollable);
}

static const char *select_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return data ? _tr("None") : _tr("All");
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    int count = 0;
    int num_telem = TELEMETRY_GetNumTelemSrc();
    int telem_delta = NUM_TELEM - num_telem;
    int absrow1 = absrow;
    if (absrow1 >= DLOG_INPUTS - telem_delta)
        absrow1 += telem_delta;
    if (absrow1 < ROW1) {
        int idx = DATALOG_BYTE(absrow1);
        int pos = DATALOG_POS(absrow1);
        GUI_CreateLabelBox(&gui->checked[relrow],
                    SCROLLABLE_X + 5, y, 16, 16,
                    (dlog->source[idx] & (1 << pos)) ? &SMALLBOXNEG_FONT : &SMALLBOX_FONT,
                    emptystr_cb, press_cb, (void *)(long)absrow1);
        GUI_CreateLabelBox(&gui->source[relrow],
                    SCROLLABLE_X + 25, y, 80, 18,
                    &DEFAULT_FONT, source_cb, NULL, (void *)(long)absrow1);
        count++;
    }
    if (absrow < ROW2) {
        int idx = DATALOG_BYTE(absrow+ROW1);
        int pos = DATALOG_POS(absrow+ROW1);
        GUI_CreateLabelBox(&gui->checked2[relrow],
                    SCROLLABLE_X + 160, y, 16, 16,
                    (dlog->source[idx] & (1 << pos)) ? &SMALLBOXNEG_FONT : &SMALLBOX_FONT,
                    emptystr_cb, press_cb, (void *)(long)(absrow+ROW1));
        GUI_CreateLabelBox(&gui->source2[relrow],
                    SCROLLABLE_X + 180, y, 80, 18, 
                    &DEFAULT_FONT, source_cb, NULL, (void *)(long)(absrow+ROW1));
        count++;
    }
    return count;
}

void PAGE_DatalogInit(int page)
{
    (void)page;
    int row = 40;
    const int ROW_HEIGHT = 20;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_DATALOG));

    //Col1
    GUI_CreateLabelBox(&gui->enlbl, SCROLLABLE_X, row, 80, 18, &DEFAULT_FONT, NULL, NULL, _tr("Enable"));
    GUI_CreateTextSource(&gui->en, SCROLLABLE_X + 78, row, TEXTSELECT_96, NULL, sourcesel_cb, sourcesel_input_cb, NULL);
    //Col2
    GUI_CreateButton(&gui->reset, SCROLLABLE_X + SCROLLABLE_WIDTH - 64, row, BUTTON_64x16, reset_str_cb, 0, reset_press_cb, NULL);
    row += ROW_HEIGHT;

    //col1
    GUI_CreateLabelBox(&gui->freqlbl, SCROLLABLE_X, row, 80, 18, &DEFAULT_FONT, NULL, NULL, _tr("Sample Rate"));
    GUI_CreateTextSelect(&gui->freq, SCROLLABLE_X + 78, row, TEXTSELECT_96, NULL, ratesel_cb, NULL);
    //col2
    GUI_CreateLabelBox(&gui->remaining, SCROLLABLE_X + 184, row, SCROLLABLE_WIDTH-200, 18, &DEFAULT_FONT, remaining_str_cb, NULL, NULL);
    row += ROW_HEIGHT;

    GUI_CreateLabelBox(&gui->selectlbl, SCROLLABLE_X, row, 80, 18, &DEFAULT_FONT, NULL, NULL, _tr("Select"));
    GUI_CreateButton(&gui->all, SCROLLABLE_X + SCROLLABLE_WIDTH - 64 - 64 - 10, row, BUTTON_64x16, select_str_cb, 0, select_press_cb, (void *)0L);
    GUI_CreateButton(&gui->none, SCROLLABLE_X + SCROLLABLE_WIDTH - 64, row, BUTTON_64x16, select_str_cb, 0, select_press_cb, (void *)1L);
    row += ROW_HEIGHT;

    int row1 = ROW1 - (NUM_TELEM - TELEMETRY_GetNumTelemSrc()); //Remove unused telemetry
    int count = row1 > ROW2 ? row1 : ROW2;
    GUI_CreateScrollable(&gui->scrollable,
         SCROLLABLE_X, row, SCROLLABLE_WIDTH, ROW_HEIGHT * DATALOG_NUM_SCROLLABLE, ROW_HEIGHT, count, row_cb, NULL, NULL, NULL);
    next_update = CLOCK_getms() / 1000 + 5;
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}
#endif //HAS_DATALOG
