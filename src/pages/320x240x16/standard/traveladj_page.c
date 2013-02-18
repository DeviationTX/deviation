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
#include "gui/gui.h"
#include "config/model.h"
#include "standard.h"
#include "../../common/standard/_traveladj_page.c"

#define gui (&gui_objs.u.stdtravel)

static void show_page(int page)
{
    struct mixer_page * mp = &pagemem.u.mixer_page;
    if (mp->firstObj) {
        GUI_RemoveHierObjects(mp->firstObj);
        mp->firstObj = NULL;       
    }   
    for (long i = 0; i < ENTRIES_PER_PAGE; i++) {
        int row = 56 + 22 * i;
        long ch = page  + i;
        if (ch >= Model.num_channels)
            break;
        MIXER_GetLimit(ch, &mp->limit);
        guiObject_t *obj = GUI_CreateLabelBox(&gui->name[i], 10, row, 0, 16, &DEFAULT_FONT, STDMIX_channelname_cb, NULL, (void *)ch);
        if (! mp->firstObj)
            mp->firstObj = obj;
        GUI_CreateTextSelect(&gui->down[i], 90, row, TEXTSELECT_96, NULL, traveldown_cb, (void *)ch);
        GUI_CreateTextSelect(&gui->up[i], 196, row, TEXTSELECT_96, NULL, travelup_cb, (void *)ch);
    }
}

void PAGE_TravelAdjInit(int page)
{
    (void)page;
    struct mixer_page * mp = &pagemem.u.mixer_page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_TRAVELADJ), MODELMENU_Show);
    mp->max_scroll = Model.num_channels > ENTRIES_PER_PAGE ?
                          Model.num_channels - ENTRIES_PER_PAGE
                        : 0;
    mp->firstObj = NULL;
    GUI_CreateScrollbar(&gui->scrollbar, 304, 32, 208, mp->max_scroll+1, NULL, STDMIX_ScrollCB, show_page);
    GUI_CreateLabelBox(&gui->dnlbl, 90, 36,  96, 16, &NARROW_FONT, NULL, NULL, _tr("Down"));
    GUI_CreateLabelBox(&gui->uplbl, 196, 36,  96, 16, &NARROW_FONT, NULL, NULL, _tr("Up"));
    show_page(page);
}
