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
#include "../../common/simple/_traveladj_page.c"

void PAGE_TravelAdjInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_TRAVELADJ), MODELMENU_Show);
    GUI_CreateLabelBox(108, 36,  96, 16, &NARROW_FONT, NULL, NULL, _tr("Down"));
    GUI_CreateLabelBox(214, 36,  96, 16, &NARROW_FONT, NULL, NULL, _tr("Up"));
    for (long i = 0; i < ENTRIES_PER_PAGE; i++) {
        int row = 56 + 20 * i;
        GUI_CreateLabelBox(10, row, 0, 16, &DEFAULT_FONT, SIMPLEMIX_channelname_cb, NULL, (void *)(i));
        GUI_CreateTextSelect(108, row, TEXTSELECT_96, 0x0000, NULL, traveldown_cb, (void *)(i));
        GUI_CreateTextSelect(214, row, TEXTSELECT_96, 0x0000, NULL, travelup_cb, (void *)(i));
    }
}
