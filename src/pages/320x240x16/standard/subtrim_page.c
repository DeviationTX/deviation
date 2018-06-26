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

#if HAS_STANDARD_GUI
#include "../../common/standard/_subtrim_page.c"

static struct stdchan_obj * const gui = &gui_objs.u.stdchan;

static void show_page(int page)
{
    enum {
        COL1 = (30 + ((LCD_WIDTH - 320) / 2)),
        COL2 = (150 + ((LCD_WIDTH - 320) / 2)),
        LABEL_WIDTH = (COL2 - COL1),
        ROW1 = (40 + ((LCD_HEIGHT - 240) / 2)),
        ROW_HEIGHT = 24,
    };
    struct mixer_page * mp = &pagemem.u.mixer_page;
    if (mp->firstObj) {
        GUI_RemoveHierObjects(mp->firstObj);
        FullRedraw = REDRAW_ONLY_DIRTY;
        mp->firstObj = NULL;
        GUI_DrawBackground(0, 32, LCD_WIDTH - 16, LCD_HEIGHT - 32);
    }
    for (long i = 0; i < ENTRIES_PER_PAGE; i++) {
        int row = ROW1 + ROW_HEIGHT * i;
        long ch = page  + i;
        if (ch >= Model.num_channels)
            break;
        mp->limit = MIXER_GetLimit(ch);
        guiObject_t *obj = GUI_CreateLabelBox(&gui->name[i], COL1, row, LABEL_WIDTH, 16, &LABEL_FONT, STDMIX_channelname_cb, NULL, (void *)(ch));
        if (! mp->firstObj)
            mp->firstObj = obj;
        GUI_CreateTextSelect(&gui->value[i], COL2, row, TEXTSELECT_128, NULL, subtrim_cb, (void *)(ch));
    }
}

void PAGE_SubtrimInit(int page)
{
    (void)page;
    struct mixer_page * mp = &pagemem.u.mixer_page;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SUBTRIM));
    mp->max_scroll = Model.num_channels > ENTRIES_PER_PAGE ?
                          Model.num_channels - ENTRIES_PER_PAGE
                        : 0;
    mp->firstObj = NULL;
    GUI_CreateScrollbar(&gui->scrollbar, LCD_WIDTH-16, 32, LCD_HEIGHT-32, mp->max_scroll+1, NULL, STDMIX_ScrollCB, show_page);
    show_page(page);
}
#endif //HAS_STANDARD_GUI
