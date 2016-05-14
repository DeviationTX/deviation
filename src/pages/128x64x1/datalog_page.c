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
static u8 seltype;
enum {
    DL_ENABLE,
    DL_RESET,
    DL_RATE,
    DL_SELECT,
    DL_SOURCE,
};

static void checkbut_press_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    long idx = (long)data;
    if (MIXER_SourceAsBoolean(Model.datalog.enable))
        return;
    dlog->source[DATALOG_BYTE(idx)] ^= (1 << DATALOG_POS(idx));
    GUI_Redraw(obj);
}

static const char *checkbut_txt_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    long idx = (long)data;
    if (dlog->source[DATALOG_BYTE(idx)] & (1 << DATALOG_POS(idx)))
       return _tr("On");
    return _tr("Off");
}

static void select_press_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    (void)data;
    if (MIXER_SourceAsBoolean(Model.datalog.enable))
        return;
    memset(&dlog->source, seltype ? 0xff : 0, sizeof(dlog->source));
    DATALOG_UpdateState();
    for (int i = 0; i < 7; i++) {
        if(OBJ_IS_USED(&gui->col2[i].but))
             GUI_Redraw(&gui->col2[i].but);
    }
}

static const char *select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    seltype = GUI_TextSelectHelper(seltype, 0, 1, dir, 1, 1, NULL);
    return seltype ? _tr("All") : _tr("None");
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)col;
    (void)data;
    return (guiObject_t *)&gui->col2[relrow].but;
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    int x = 63;
    int w = 59;
    const void *lbl_data = NULL;
    void *lbl_cb = NULL;
    void *but_press = NULL;
    const void *but_data = NULL;
    void *but_txt = NULL;
    void *sel_cb = NULL;
    void *sel_input_cb = NULL;
    void *sel_data = NULL;
    void *selpress_cb = NULL;

    if (absrow == DL_ENABLE) {
        lbl_cb = NULL; lbl_data = _tr("Enable");
        sel_cb = sourcesel_cb; sel_data = NULL; sel_input_cb = sourcesel_input_cb;
    } else if (absrow == DL_RESET) {
        lbl_data = NULL;
        but_press = reset_press_cb; but_txt = reset_str_cb;
    } else if (absrow == DL_RATE) {
        lbl_cb = NULL; lbl_data = _tr("Rate");
        sel_cb = ratesel_cb; sel_data = NULL;
    } else if (absrow == DL_SELECT) {
        lbl_cb = NULL; lbl_data = _tr("Select");
        selpress_cb = select_press_cb; sel_cb = select_cb; sel_data = NULL;
    } else {
        long row = absrow-DL_SOURCE;
        int num_telem = TELEMETRY_GetNumTelemSrc();
        int telem_delta = NUM_TELEM - num_telem;
        if (row >= DLOG_INPUTS - telem_delta)
            row += telem_delta;
        lbl_cb = source_cb; lbl_data = (void *)row;
        but_press = checkbut_press_cb; but_txt = checkbut_txt_cb;
        but_data = (void *)row;
    }
    if (lbl_data || lbl_cb) {
        GUI_CreateLabelBox(&gui->label[relrow], 0, y,
            0, LINE_HEIGHT, &DEFAULT_FONT, lbl_cb, NULL, lbl_data);
    }
    if (but_press) {
        GUI_CreateButtonPlateText(&gui->col2[relrow].but, x, y,
            w, LINE_HEIGHT, &DEFAULT_FONT, but_txt, 0x0000, but_press, but_data);
    } else {
        GUI_CreateTextSourcePlate(&gui->col2[relrow].ts, x, y,
            w, LINE_HEIGHT, &DEFAULT_FONT, selpress_cb, sel_cb, sel_input_cb, sel_data);
    }
    return 1;
}

static const char *remaining_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    snprintf(tempstring, sizeof(tempstring), _tr("%d bytes left"), DATALOG_Remaining());
    return tempstring;
}

void PAGE_DatalogInit(int page)
{
    (void)page;
    memset(gui, 0, sizeof(*gui));
    seltype = 0;
    struct LabelDesc font = DEFAULT_FONT;
    font.style = LABEL_UNDERLINE;
    int count = DL_SOURCE + DLOG_LAST - (NUM_TELEM - TELEMETRY_GetNumTelemSrc()); //Remove unused telemetry
    GUI_CreateLabelBox(&gui->remaining, 0, 0,
        LCD_WIDTH-1, LINE_HEIGHT, &font, remaining_str_cb, NULL, NULL);
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, count, row_cb, getobj_cb, NULL, NULL);
    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

#endif //HAS_DATLOG
