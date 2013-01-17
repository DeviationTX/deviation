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
    int y = 40;
    int height = 20;
    //Row 1
    GUI_CreateLabel(&gui->reverselbl, 8, y, NULL, DEFAULT_FONT, _tr("Reverse:"));
    GUI_CreateTextSelect(&gui->reverse, 128, y, TEXTSELECT_96, 0x0000, toggle_reverse_cb, reverse_cb, (void *)((long)mp->channel));
    y += height;
    //Row 2
    GUI_CreateLabel(&gui->failsafelbl, 8, y, NULL, DEFAULT_FONT, _tr("Failsafe:"));
    GUI_CreateTextSelect(&gui->failsafe, 128, y, TEXTSELECT_96, 0x0000, toggle_failsafe_cb, set_failsafe_cb, NULL);
    y += height;
    //Row 3
    GUI_CreateLabel(&gui->safetylbl, 8, y, NULL, DEFAULT_FONT, _tr("Safety:"));
    GUI_CreateTextSelect(&gui->safety, 128, y, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->limit.safetysw);
    y += height;
    //Row 4
    GUI_CreateLabel(&gui->safevallbl, 8, y, NULL, DEFAULT_FONT, _tr("Safe Val:"));
    GUI_CreateTextSelect(&gui->safeval, 128, y, TEXTSELECT_96, 0x0000, NULL, set_safeval_cb, NULL);
    y += height;
    //Row 5
    GUI_CreateLabel(&gui->minlbl, 8, y, NULL, DEFAULT_FONT, _tr("Min Limit:"));
    GUI_CreateTextSelect(&gui->min, 128, y, TEXTSELECT_96, 0x0000, NULL, set_limits_cb, &mp->limit.min);
    y += height;
    //Row 6
    GUI_CreateLabel(&gui->maxlbl, 8, y, NULL, DEFAULT_FONT, _tr("Max Limit:"));
    GUI_CreateTextSelect(&gui->max, 128, y, TEXTSELECT_96, 0x0000, NULL, set_limits_cb, &mp->limit.max);
    y += height;
    //Row 5
    GUI_CreateLabel(&gui->scalelbl, 8, y, scalestring_cb, DEFAULT_FONT, (void *)1L);
    GUI_CreateTextSelect(&gui->scale, 128, y, TEXTSELECT_96, 0x0000, NULL, set_limitsscale_cb, &mp->limit.servoscale);
    y += height;
    GUI_CreateLabel(&gui->scaleneglbl, 8, y, scalestring_cb, DEFAULT_FONT, (void *)0L);
    GUI_CreateTextSelect(&gui->scaleneg, 128, y, TEXTSELECT_96, 0x0000, NULL, set_limitsscale_cb, &mp->limit.servoscale_neg);
    y += height;
    //Row 6
    GUI_CreateLabel(&gui->subtrimlbl, 8, y, NULL, DEFAULT_FONT, _tr("Subtrim:"));
    GUI_CreateTextSelect(&gui->subtrim, 128, y, TEXTSELECT_96, 0x0000, NULL, set_trimstep_cb, &mp->limit.subtrim);
    y += height;
    //Row 7
    GUI_CreateLabel(&gui->speedlbl, 8, y, NULL, DEFAULT_FONT, _tr("Speed:"));
    GUI_CreateTextSelect(&gui->speed, 128, y, TEXTSELECT_96, 0x0000, NULL, set_limits_cb, &mp->limit.speed);
}

static void _show_titlerow()
{
    GUI_CreateLabel(&gui->title, 8, 10, MIXPAGE_ChanNameProtoCB, TITLE_FONT, (void *)(long)mp->channel);
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
}

static inline guiObject_t *_get_obj(int idx, int objid) {
    (void)objid;
    switch(idx) {
        case ITEM_SAFEVAL: return (guiObject_t *)&gui->safeval;
        case ITEM_SCALENEG: return (guiObject_t *)&gui->scaleneg;
        default: return NULL;
    }
}
