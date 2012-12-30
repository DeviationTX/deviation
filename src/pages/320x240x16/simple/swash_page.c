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
#include "mixer.h"
#include "simple.h"
#include "mixer_simple.h"

#include "../../common/simple/_swash_page.c"

void PAGE_SwashInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_SWASH), MODELMENU_Show);
    get_swash();
    #define COL1 10
    #define COL2 150
    #define ROW_SPACE 20
    /* Row 1 */
    int row = 40;
    GUI_CreateLabelBox(COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("SwashType:"));
    GUI_CreateTextSelect(COL2, row, TEXTSELECT_96, 0x0000, NULL, swash_val_cb, NULL);

    /* Row 2 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("ELE Mix:"));
    itemObj[0] = GUI_CreateTextSelect(COL2, row, TEXTSELECT_96, 0x0000, NULL, swashmix_val_cb, (void *)1L);

    /* Row 3 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("AIL Mix:"));
    itemObj[1] = GUI_CreateTextSelect(COL2, row, TEXTSELECT_96, 0x0000, NULL, swashmix_val_cb, (void *)0);

    /* Row 4 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Pit Mix:"));
    itemObj[2] = GUI_CreateTextSelect(COL2, row, TEXTSELECT_96, 0x0000, NULL, swashmix_val_cb, (void *)2);

    update_swashmixes();
}

