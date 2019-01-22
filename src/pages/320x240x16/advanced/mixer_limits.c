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
#include <stdlib.h>

#include "../../common/advanced/_mixer_limits.c"


static void _show_limits()
{
    enum {
        COL1 = (8 + ((LCD_WIDTH - 320) / 2)),
        COL2 = (COL1 + 120),
        ROW1 = (40 + ((LCD_HEIGHT - 240) / 2)),
        LABEL_WIDTH = (COL2 - COL1),
    };
    int y = ROW1;
    int height = 20;
    //Row 1
    GUI_CreateLabelBox(&gui->reverselbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Reverse"));
    GUI_CreateTextSelect(&gui->reverse, COL2, y, TEXTSELECT_96, toggle_reverse_cb, reverse_cb, (void *)((long)mp->channel));
    y += height;
    //Row 2
    GUI_CreateLabelBox(&gui->failsafelbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Fail-safe"));
    GUI_CreateTextSelect(&gui->failsafe, COL2, y, TEXTSELECT_96, toggle_failsafe_cb, set_failsafe_cb, NULL);
    y += height;
    //Row 3
    GUI_CreateLabelBox(&gui->safetylbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Safety"));
    GUI_CreateTextSource(&gui->safety, COL2, y, TEXTSELECT_96, sourceselect_cb, set_source_cb, set_input_source_cb, &mp->limit->safetysw);
    y += height;
    //Row 4
    GUI_CreateLabelBox(&gui->safevallbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Safe Val"));
    GUI_CreateTextSelect(&gui->safeval, COL2, y, TEXTSELECT_96, NULL, set_safeval_cb, NULL);
    y += height;
    //Row 5
    GUI_CreateLabelBox(&gui->minlbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Min Limit"));
    GUI_CreateTextSelect(&gui->min, COL2, y, TEXTSELECT_96, NULL, set_limits_cb, &mp->limit->min);
    y += height;
    //Row 6
    GUI_CreateLabelBox(&gui->maxlbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Max Limit"));
    GUI_CreateTextSelect(&gui->max, COL2, y, TEXTSELECT_96, NULL, set_limits_cb, &mp->limit->max);
    y += height;
    //Row 5
    GUI_CreateLabelBox(&gui->scaleneglbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, scalestring_cb, NULL, (void *)0L);
    GUI_CreateTextSelect(&gui->scaleneg, COL2, y, TEXTSELECT_96, NULL, set_limitsscale_cb, &mp->limit->servoscale_neg);
    y += height;
    GUI_CreateLabelBox(&gui->scalelbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, scalestring_cb, NULL, (void *)1L);
    GUI_CreateTextSelect(&gui->scale, COL2, y, TEXTSELECT_96, NULL, set_limitsscale_cb, &mp->limit->servoscale);
    y += height;
    //Row 6
    GUI_CreateLabelBox(&gui->subtrimlbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Subtrim"));
    GUI_CreateTextSelect(&gui->subtrim, COL2, y, TEXTSELECT_96, NULL, set_trimstep_cb, &mp->limit->subtrim);
    y += height;
    //Row 7
    GUI_CreateLabelBox(&gui->speedlbl, COL1, y, LABEL_WIDTH, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Speed"));
    GUI_CreateTextSelect(&gui->speed, COL2, y, TEXTSELECT_96, NULL, set_limits_cb, &mp->limit->speed);
}

static void _show_titlerow()
{
    enum {
        TITLE_X     = 40,
        BUTTON_X    = LCD_WIDTH - 96 - 8,
        TITLE_WIDTH = BUTTON_X - TITLE_X,
    };
    PAGE_ShowHeader(NULL);
    GUI_CreateLabelBox(&gui->title, TITLE_X, 10, TITLE_WIDTH, 12, &TITLE_FONT, MIXPAGE_ChanNameProtoCB, NULL, (void *)(long)mp->channel);
    GUI_CreateButton(&gui->revert, BUTTON_X, 4, BUTTON_96, GUI_Localize, revert_cb, _tr_noop("Revert"));
}

static inline guiObject_t *_get_obj(int idx, int objid) {
    (void)objid;
    switch(idx) {
        case ITEM_SAFEVAL: return (guiObject_t *)&gui->safeval;
        case ITEM_SCALENEG: return (guiObject_t *)&gui->scaleneg;
        default: return NULL;
    }
}
