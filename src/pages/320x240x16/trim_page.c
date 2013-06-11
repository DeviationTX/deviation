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

#define gui (&gui_objs.u.trim)
#define gui_ed (&gui_objs.u.trimedit)
static void _show_page()
{
    if (Model.mixer_mode == MIXER_STANDARD)
        PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_TRIM), MODELMENU_Show);
    else
        PAGE_ShowHeader(PAGE_GetName(PAGEID_TRIM));
    GUI_CreateLabelBox(&gui->inplbl, 4, 40, 64, 15, &NARROW_FONT, NULL, NULL, _tr("Input"));
    GUI_CreateLabelBox(&gui->neglbl, 74, 40, 64, 15, &NARROW_FONT, NULL, NULL, _tr("Trim -"));
    GUI_CreateLabelBox(&gui->poslbl, 138, 40, 64, 15, &NARROW_FONT, NULL, NULL, _tr("Trim +"));
    GUI_CreateLabelBox(&gui->steplbl, 202, 40, 108, 15, &NARROW_FONT, NULL, NULL, _tr("Trim Step"));
    struct Trim *trim = MIXER_GetAllTrims();
    for (u8 i = 0; i < NUM_TRIMS; i++) {
        GUI_CreateButton(&gui->src[i], 4, 24*i + 66, BUTTON_64x16,
            trimsource_name_cb, 0x0000, _edit_cb, (void *)((long)i));
        GUI_CreateLabel(&gui->neg[i], 80, 24*i + 68, NULL, DEFAULT_FONT, (void *)INPUT_ButtonName(trim[i].neg));
        GUI_CreateLabel(&gui->pos[i], 144, 24*i + 68, NULL, DEFAULT_FONT, (void *)INPUT_ButtonName(trim[i].pos));
        GUI_CreateTextSelect(&gui->step[i], 208, 24*i + 66, TEXTSELECT_96, NULL, set_trimstep_cb, &trim[i].step);
    }
}

static void _edit_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    struct Trim *trim = MIXER_GetAllTrims();
    PAGE_SetModal(1);
    tp->index = (long)data;
    tp->trim = trim[tp->index];

    PAGE_RemoveAllObjects();
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);

    //Row 1
    GUI_CreateLabel(&gui_ed->srclbl, 8, 48, NULL, DEFAULT_FONT, _tr("Input"));
    GUI_CreateTextSelect(&gui_ed->src, 104, 48, TEXTSELECT_96, NULL, set_source_cb, &tp->trim.src);
    //Row 2
    GUI_CreateLabel(&gui_ed->neglbl, 8, 72, NULL, DEFAULT_FONT, _tr("Trim +"));
    GUI_CreateTextSelect(&gui_ed->neg, 104, 72, TEXTSELECT_96, NULL, set_trim_cb, &tp->trim.pos);
    //Row 3
    GUI_CreateLabel(&gui_ed->poslbl, 8, 96, NULL, DEFAULT_FONT, _tr("Trim -"));
    GUI_CreateTextSelect(&gui_ed->pos, 104, 96, TEXTSELECT_96, NULL, set_trim_cb, &tp->trim.neg);
    //Row 4
    GUI_CreateLabel(&gui_ed->steplbl, 8, 120, NULL, DEFAULT_FONT, _tr("Trim Step"));
    GUI_CreateTextSelect(&gui_ed->step, 104, 120, TEXTSELECT_96, NULL, set_trimstep_cb, &tp->trim.step);
    //Row 5
    GUI_CreateLabel(&gui_ed->swlbl, 8, 144, NULL, DEFAULT_FONT, _tr("Switch"));
    GUI_CreateTextSelect(&gui_ed->sw, 104, 144, TEXTSELECT_96, NULL, set_switch_cb, &tp->trim.sw);
}
