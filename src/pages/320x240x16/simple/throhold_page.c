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
#include "../../common/simple/_throhold_page.c"

void PAGE_ThroHoldInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_THROHOLD), MODELMENU_Show);
    GUI_CreateLabelBox(30, 80, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Thr hold"));
    GUI_CreateTextSelect(150, 80, TEXTSELECT_128, 0x0000, NULL, throhold_cb,  NULL);

    GUI_CreateLabelBox(30, 120, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Hold position"));
    mp->itemObj[0] = GUI_CreateTextSelect(150, 120, TEXTSELECT_128, 0x0000, NULL, holdpostion_cb,  NULL);
}
