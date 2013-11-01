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

#if DATALOG_ENABLED
#include "../common/_datalog_page.c"


#define ROW1 (DLOG_CHANNELS)
#define ROW2 (DLOG_LAST - DLOG_CHANNELS)
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
    if (DATALOG_IsEnabled())
        return;
    if (press == 0) {
        dlog.source[DATALOG_BYTE(absrow)] ^= (1 << DATALOG_POS(absrow));
        GUI_SetLabelDesc((guiLabel_t *)obj,
               (dlog.source[DATALOG_BYTE(absrow)] & (1 << DATALOG_POS(absrow)))
               ? &SMALLBOXNEG_FONT
               : &SMALLBOX_FONT);
        DATALOG_UpdateState();
    }
}

static const char *remaining_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    sprintf(str, _tr("%d bytes left"), DATALOG_Remaining());
    return str;
}

static void select_press_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    (void)data;
    if (DATALOG_IsEnabled())
        return;
    memset(&dlog.source, data ? 0 : 0xff, sizeof(dlog.source));
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

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)col;
    (void)data;
    if (OBJ_IS_USED(&gui->checked[relrow]) && OBJ_IS_USED(&gui->checked2[relrow])) {
        col = (col + 2) % 2;
        return (guiObject_t *)(col ? &gui->checked2[relrow] : &gui->checked[relrow]);
    } else if (OBJ_IS_USED(&gui->checked[relrow])) {
        return (guiObject_t *)&gui->checked[relrow];
    } else {
        return (guiObject_t *)&gui->checked2[relrow];
    }
}

#define SCROLLABLE_WIDTH (275 + 16 + 5)
#define SCROLLABLE_X (LCD_WIDTH - SCROLLABLE_WIDTH) / 2
static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    int count = 0;
    if (absrow < ROW1) {
        int idx = DATALOG_BYTE(absrow);
        int pos = DATALOG_POS(absrow);
        GUI_CreateLabelBox(&gui->checked[relrow],
                    SCROLLABLE_X + 5, y, 16, 16,
                    (dlog.source[idx] & (1 << pos)) ? &SMALLBOXNEG_FONT : &SMALLBOX_FONT,
                    emptystr_cb, press_cb, (void *)(long)absrow);
        GUI_CreateLabelBox(&gui->source[relrow],
                    SCROLLABLE_X + 25, y, 16, 100,
                    &DEFAULT_FONT, source_cb, NULL, (void *)(long)absrow);
        count++;
    }
    if (absrow < ROW2) {
        int idx = DATALOG_BYTE(absrow+ROW1);
        int pos = DATALOG_POS(absrow+ROW1);
        GUI_CreateLabelBox(&gui->checked2[relrow],
                    SCROLLABLE_X + 160, y, 16, 16,
                    (dlog.source[idx] & (1 << pos)) ? &SMALLBOXNEG_FONT : &SMALLBOX_FONT,
                    emptystr_cb, press_cb, (void *)(long)(absrow+ROW1));
        GUI_CreateLabelBox(&gui->source2[relrow],
                    SCROLLABLE_X + 180, y, 16, 100,
                    &DEFAULT_FONT, source_cb, NULL, (void *)(long)(absrow+ROW1));
        count++;
    }
    return count;
}

void PAGE_DatalogInit(int page)
{
    (void)page;
    int row = 40;
    #define ROW_HEIGHT 20
    if (Model.mixer_mode == MIXER_STANDARD)
        PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_DATALOG), MODELMENU_Show);
    else
        PAGE_ShowHeader(PAGE_GetName(PAGEID_DATALOG));

    //Col1
    GUI_CreateLabelBox(&gui->enlbl, SCROLLABLE_X, row, 80, 20, &DEFAULT_FONT, NULL, NULL, _tr("Enable"));
    GUI_CreateTextSelect(&gui->en, SCROLLABLE_X + 78, row, TEXTSELECT_96, NULL, sourcesel_cb, NULL);
    //Col2
    GUI_CreateButton(&gui->reset, SCROLLABLE_X + SCROLLABLE_WIDTH - 64, row, BUTTON_64x16, reset_str_cb, 0, reset_press_cb, NULL);
    row += ROW_HEIGHT;

    //col1
    GUI_CreateLabelBox(&gui->freqlbl, SCROLLABLE_X, row, 80, 20, &DEFAULT_FONT, NULL, NULL, _tr("Sample Rate"));
    GUI_CreateTextSelect(&gui->freq, SCROLLABLE_X + 78, row, TEXTSELECT_96, NULL, ratesel_cb, NULL);
    //col2
    GUI_CreateLabelBox(&gui->remaining, SCROLLABLE_X + 184, row, SCROLLABLE_WIDTH-200, 20, &DEFAULT_FONT, remaining_str_cb, NULL, NULL);
    row += ROW_HEIGHT;

    GUI_CreateLabelBox(&gui->selectlbl, SCROLLABLE_X, row, 80, 20, &DEFAULT_FONT, NULL, NULL, _tr("Select"));
    GUI_CreateButton(&gui->all, SCROLLABLE_X + SCROLLABLE_WIDTH - 64 - 64 - 10, row, BUTTON_64x16, select_str_cb, 0, select_press_cb, (void *)0L);
    GUI_CreateButton(&gui->none, SCROLLABLE_X + SCROLLABLE_WIDTH - 64, row, BUTTON_64x16, select_str_cb, 0, select_press_cb, (void *)1L);
    row += ROW_HEIGHT;

    int count = ROW1 > ROW2 ? ROW1 : ROW2;
    GUI_CreateScrollable(&gui->scrollable,
         SCROLLABLE_X, row, SCROLLABLE_WIDTH, ROW_HEIGHT * DATALOG_NUM_SCROLLABLE, ROW_HEIGHT, count, row_cb, getobj_cb, NULL, NULL);
    next_update = CLOCK_getms() / 1000 + 5;
}
#endif //DATALOG_ENABLED
