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
#include "standard.h"
#include "mixer_standard.h"

#include "../../common/standard/_swash_page.c"

#define gui (&gui_objs.u.stdswash)

void PAGE_SwashInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_SWASH), MODELMENU_Show);
    get_swash();
    #define COL1 (10 + ((LCD_WIDTH - 320) / 2))
    #define COL2 (150 + ((LCD_WIDTH - 320) / 2))
    #define ROW_SPACE 30
    /* Row 1 */
    int row = 40 + ((LCD_HEIGHT - 240) / 2);
    GUI_CreateLabelBox(&gui->typelbl, COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("SwashType"));
    GUI_CreateTextSelect(&gui->type, COL2, row, TEXTSELECT_96, NULL, swash_val_cb, NULL);

    /* Row 2 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->lbl[0], COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("ELE Mix"));
    GUI_CreateTextSelect(&gui->mix[0], COL2, row, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)1L);

    /* Row 3 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->lbl[1], COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("AIL Mix"));
    GUI_CreateTextSelect(&gui->mix[1], COL2, row, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)0);

    /* Row 4 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->lbl[2], COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("PIT Mix"));
    GUI_CreateTextSelect(&gui->mix[2], COL2, row, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)2);

    update_swashmixes();
}

