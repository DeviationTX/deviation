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
#include <stdlib.h>

#include "../common/_mixer_limits.c"


static void _show_limits()
{
    int y = 40;
    int height = 20;
    //Row 1
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Reverse:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, toggle_reverse_cb, reverse_cb, (void *)((long)mp->channel));
    y += height;
    //Row 2
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Failsafe:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, toggle_failsafe_cb, set_failsafe_cb, NULL);
    y += height + 6;
    //Row 3
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Safety:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->limit.safetysw);
    y += height;
    //Row 4
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Safe Val:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, NULL, PAGEMIXER_SetNumberCB, &mp->limit.safetyval);
    y += height + 6;
    //Row 5
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Min:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, NULL, set_limits_cb, &mp->limit.min);
    y += height;
    //Row 6
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Max:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, NULL, set_limits_cb, &mp->limit.max);
    y += height;
    //Row 5
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Scale:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, NULL, set_limits_cb, &mp->limit.servoscale);
    y += height + 6;
    //Row 6
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Subtrim:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, NULL, set_trimstep_cb, &mp->limit.subtrim);
    y += height;
    //Row 7
    GUI_CreateLabel(8, y, NULL, DEFAULT_FONT, _tr("Speed:"));
    GUI_CreateTextSelect(128, y, TEXTSELECT_96, 0x0000, NULL, set_limits_cb, &mp->limit.speed);
}

static void _show_titlerow()
{
    titleObj = GUI_CreateLabel(8, 10, MIXPAGE_ChanNameProtoCB, TITLE_FONT, (void *)(long)mp->channel);
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
}
