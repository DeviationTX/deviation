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

#include "../common/_trim_page.c"

static struct trim_obj     * const gui    = &gui_objs.u.trim;
static struct trimedit_obj * const gui_ed = &gui_objs.u.trimedit;

enum {
     PCOL1 = (4 + ((LCD_WIDTH - 320) / 2)),
     PCOL2 = (72 + ((LCD_WIDTH - 320) / 2)),
     PCOL3 = (134 + ((LCD_WIDTH - 320) / 2)),
     PCOL4 = (196 + ((LCD_WIDTH - 320) / 2)),
     PROW1 = (40 + ((LCD_HEIGHT - 240) / 2)),
     PROW2 = (64 + ((LCD_HEIGHT - 240) / 2)),
     PROW3 = (PROW2 + 2),
};

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    col = (col + 2) % 2;
    return col ? (guiObject_t *)&gui->step[relrow] : (guiObject_t *)&gui->src[relrow];
}
static const char *negtrim_str(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    if(Model.trims[i].step == TRIM_MOMENTARY || Model.trims[i].step == TRIM_TOGGLE)
        return _tr("None");
    struct Trim *trim = MIXER_GetAllTrims();
    return INPUT_ButtonName(trim[i].neg);
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    struct Trim *trim = MIXER_GetAllTrims();
    GUI_CreateButton(&gui->src[relrow], PCOL1, y, BUTTON_64x16,
        trimsource_name_cb, edit_trim_cb, (void *)((long)absrow));
    GUI_CreateLabel(&gui->neg[relrow], PCOL2 + 6, y, negtrim_str, DEFAULT_FONT, (void *)(long)absrow);
    GUI_CreateLabel(&gui->pos[relrow], PCOL3 + 6, y, NULL, DEFAULT_FONT, (void *)INPUT_ButtonName(trim[absrow].pos));
    GUI_CreateTextSelect(&gui->step[relrow], PCOL4 + 6, y, TEXTSELECT_96, NULL,
                         set_trimstep_cb, (void *)(long)(absrow + 0x000)); //0x000: Use Model.trims
    return 2;
}

static void _show_page()
{
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TRIM));
    GUI_CreateLabelBox(&gui->inplbl, PCOL1, PROW1, 64, 15, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Input"));
    GUI_CreateLabelBox(&gui->neglbl, PCOL2, PROW1, 64, 15, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Trim -"));
    GUI_CreateLabelBox(&gui->poslbl, PCOL3, PROW1, 64, 15, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Trim +"));
    GUI_CreateLabelBox(&gui->steplbl, PCOL4, PROW1, 108, 15, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Trim Step"));

    GUI_CreateScrollable(&gui->scrollable,
         PCOL1, PROW2,  LCD_WIDTH - 2 * PCOL1, NUM_TRIM_ROWS * 24 - 8,
         24, NUM_TRIMS, row_cb, getobj_cb, NULL, NULL);

    PAGE_SetScrollable(&gui->scrollable, &current_selected);
}

void PAGE_TrimEditInit(int page)
{
    struct Trim *trim = MIXER_GetAllTrims();
    tp->index = page;
    tp->trim = trim[tp->index];

    //PAGE_ShowHeader(_tr("Trim"));
    PAGE_ShowHeaderWithSize(_tr("Trim"), LCD_WIDTH - 96, 0);
    PAGE_CreateOkButton(LCD_WIDTH-56, 4, okcancel_cb);

    enum {
         COL1 = (64 + ((LCD_WIDTH - 320) / 2)),
         COL2 = (160 + ((LCD_WIDTH - 320) / 2)),
         ROW1 = (80 + ((LCD_HEIGHT - 240) / 2)),
         ROW2 = (ROW1 + 24),
         ROW3 = (ROW1 + 48),
         ROW4 = (ROW1 + 72),
         ROW5 = (ROW1 + 96),
    };

    //Row 1
    GUI_CreateLabelBox(&gui_ed->srclbl, COL1, ROW1, COL2-COL1, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Input"));
    GUI_CreateTextSource(&gui_ed->src, COL2, ROW1, TEXTSELECT_96, NULL, set_source_cb, set_input_source_cb, &tp->trim.src);
    //Row 2
    GUI_CreateLabelBox(&gui_ed->steplbl, COL1, ROW2, COL2-COL1, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Trim Step"));
    GUI_CreateTextSelect(&gui_ed->step, COL2, ROW2, TEXTSELECT_96, NULL,
                         set_trimstep_cb, (void *)(long)(tp->index + 0x100)); //0x100: Use tp->trim
    //Row 3
    GUI_CreateLabelBox(&gui_ed->neglbl, COL1, ROW3, COL2-COL1, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Trim -"));
    GUI_CreateTextSelect(&gui_ed->neg, COL2, ROW3, TEXTSELECT_96, NULL, set_trim_cb, &tp->trim.neg);
    //Row 4
    GUI_CreateLabelBox(&gui_ed->poslbl, COL1, ROW4, COL2-COL1, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Trim +"));
    GUI_CreateTextSelect(&gui_ed->pos, COL2, ROW4, TEXTSELECT_96, NULL, set_trim_cb, &tp->trim.pos);
    //Row 5
    GUI_CreateLabelBox(&gui_ed->swlbl, COL1, ROW5, COL2-COL1, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Switch"));
    GUI_CreateTextSource(&gui_ed->sw, COL2, ROW5, TEXTSELECT_96, NULL, set_switch_cb, set_input_switch_cb, &tp->trim.sw);
}

static inline guiObject_t * _get_obj(int idx, int objid) {
    if (PAGE_GetModal()) {
        if(objid == TRIM_MINUS) {
            return (guiObject_t *)&gui_ed->neg;
        } else if(objid == TRIM_SWITCH) {
            return (guiObject_t *)&gui_ed->sw;
        }
    } else {
        if(objid == TRIM_MINUS) {
            return (guiObject_t *)&gui->neg[idx - GUI_ScrollableCurrentRow(&gui->scrollable)];
        }
    }
    return NULL;

}
