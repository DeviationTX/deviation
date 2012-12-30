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
#include "simple.h"
#include "../../common/simple/_failsafe_page.c"

static void show_page(int page)
{
    struct mixer_page * mp = &pagemem.u.mixer_page;
    if (mp->firstObj) {
        GUI_RemoveHierObjects(mp->firstObj);
        mp->firstObj = NULL;       
    }   
    for (long i = 0; i < ENTRIES_PER_PAGE; i++) {
        int row = 40 + 24 * i;
        long ch = page  + i;
        if (ch >= Model.num_channels)
            break;
        MIXER_GetLimit(ch, &mp->limit);
        guiObject_t *obj = GUI_CreateLabelBox(30, row, 0, 16, &DEFAULT_FONT, SIMPLEMIX_channelname_cb, NULL, (void *)(ch));
        if (! mp->firstObj)
            mp->firstObj = obj;
        GUI_CreateTextSelect(150, row, TEXTSELECT_128, 0x0000, toggle_failsafe_cb, set_failsafe_cb, (void *)ch);
    }
}
void PAGE_FailSafeInit(int page)
{
    (void)page;
    struct mixer_page * mp = &pagemem.u.mixer_page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_FAILSAFE), MODELMENU_Show);
    mp->max_scroll = Model.num_channels > ENTRIES_PER_PAGE ?
                          Model.num_channels - ENTRIES_PER_PAGE
                        : 0;
    mp->firstObj = NULL;
    GUI_CreateScrollbar(304, 32, 208, mp->max_scroll+1, NULL, SIMPLEMIX_ScrollCB, show_page);
    show_page(page);
}
