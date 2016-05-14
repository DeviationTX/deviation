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
    };
    int y = ROW1;
    int height = 20;
    //Row 1
    GUI_CreateLabel(&gui->reverselbl, COL1, y, NULL, DEFAULT_FONT, _tr("Reverse"));
    GUI_CreateTextSelect(&gui->reverse, COL2, y, TEXTSELECT_96, toggle_reverse_cb, reverse_cb, (void *)((long)mp->channel));
    y += height;
    //Row 2
    GUI_CreateLabel(&gui->failsafelbl, COL1, y, NULL, DEFAULT_FONT, _tr("Fail-safe"));
    GUI_CreateTextSelect(&gui->failsafe, COL2, y, TEXTSELECT_96, toggle_failsafe_cb, set_failsafe_cb, NULL);
    y += height;
    //Row 3
    GUI_CreateLabel(&gui->safetylbl, COL1, y, NULL, DEFAULT_FONT, _tr("Safety"));
    GUI_CreateTextSource(&gui->safety, COL2, y, TEXTSELECT_96, sourceselect_cb, set_source_cb, set_input_source_cb, &mp->limit->safetysw);
    y += height;
    //Row 4
    GUI_CreateLabel(&gui->safevallbl, COL1, y, NULL, DEFAULT_FONT, _tr("Safe Val"));
    GUI_CreateTextSelect(&gui->safeval, COL2, y, TEXTSELECT_96, NULL, set_safeval_cb, NULL);
    y += height;
    //Row 5
    GUI_CreateLabel(&gui->minlbl, COL1, y, NULL, DEFAULT_FONT, _tr("Min Limit"));
    GUI_CreateTextSelect(&gui->min, COL2, y, TEXTSELECT_96, NULL, set_limits_cb, &mp->limit->min);
    y += height;
    //Row 6
    GUI_CreateLabel(&gui->maxlbl, COL1, y, NULL, DEFAULT_FONT, _tr("Max Limit"));
    GUI_CreateTextSelect(&gui->max, COL2, y, TEXTSELECT_96, NULL, set_limits_cb, &mp->limit->max);
    y += height;
    //Row 5
    GUI_CreateLabel(&gui->scaleneglbl, COL1, y, scalestring_cb, DEFAULT_FONT, (void *)0L);
    GUI_CreateTextSelect(&gui->scaleneg, COL2, y, TEXTSELECT_96, NULL, set_limitsscale_cb, &mp->limit->servoscale_neg);
    y += height;
    GUI_CreateLabel(&gui->scalelbl, COL1, y, scalestring_cb, DEFAULT_FONT, (void *)1L);
    GUI_CreateTextSelect(&gui->scale, COL2, y, TEXTSELECT_96, NULL, set_limitsscale_cb, &mp->limit->servoscale);
    y += height;
    //Row 6
    GUI_CreateLabel(&gui->subtrimlbl, COL1, y, NULL, DEFAULT_FONT, _tr("Subtrim"));
    GUI_CreateTextSelect(&gui->subtrim, COL2, y, TEXTSELECT_96, NULL, set_trimstep_cb, &mp->limit->subtrim);
    y += height;
    //Row 7
    GUI_CreateLabel(&gui->speedlbl, COL1, y, NULL, DEFAULT_FONT, _tr("Speed"));
    GUI_CreateTextSelect(&gui->speed, COL2, y, TEXTSELECT_96, NULL, set_limits_cb, &mp->limit->speed);
}

const char * revert_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Revert");
}

static void _show_titlerow()
{
    PAGE_ShowHeader(NULL);
    GUI_CreateLabel(&gui->title, 40, 10, MIXPAGE_ChanNameProtoCB, TITLE_FONT, (void *)(long)mp->channel);
    GUI_CreateButton(&gui->revert, LCD_WIDTH-96-8, 4, BUTTON_96, revert_str_cb, 0x0000, revert_cb, NULL);
}

static inline guiObject_t *_get_obj(int idx, int objid) {
    (void)objid;
    switch(idx) {
        case ITEM_SAFEVAL: return (guiObject_t *)&gui->safeval;
        case ITEM_SCALENEG: return (guiObject_t *)&gui->scaleneg;
        default: return NULL;
    }
}
