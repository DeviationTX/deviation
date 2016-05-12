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
#include "../../common/standard/_switchassign_page.c"

static const char *switch_cb2(guiObject_t *obj, int dir, void *data)
{
    const char *str = switch_cb(obj, dir, data);
    if (dir)
        save_changes();
    return str;
}

void PAGE_SwitchAssignInit(int page)
{
    (void)page;
    const int COL1 = (10 + ((LCD_WIDTH - 320) / 2));
    const int COL2 = (150 + ((LCD_WIDTH - 320) / 2));
    const int ROW_SPACE = 30;

    PAGE_ShowHeader(PAGE_GetName(PAGEID_SWITCHASSIGN));
    refresh_switches();

    /* Row 1 */
    int row = 40 + ((LCD_HEIGHT - 240) / 2);
    GUI_CreateLabelBox(&gui->modelbl, COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Fly mode"));
    GUI_CreateTextSelect(&gui->mode, COL2, row, TEXTSELECT_128, NULL, switch_cb2, (void *)(long)SWITCHFUNC_FLYMODE);

    /* Row 2 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->tholdlbl, COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Thr hold"));
    GUI_CreateTextSelect(&gui->thold, COL2, row, TEXTSELECT_128, NULL, switch_cb2, (void *)(long)SWITCHFUNC_HOLD);

    /* Row 3 */
    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->gyrolbl, COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("Gyro sense"));
    GUI_CreateTextSelect(&gui->gyro, COL2, row, TEXTSELECT_128, NULL, switch_cb2, (void *)(long)SWITCHFUNC_GYROSENSE);
    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->draillbl, COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("D/R&Exp -AIL"));
    GUI_CreateTextSelect(&gui->drail, COL2, row, TEXTSELECT_128, NULL, switch_cb2, (void *)(long)SWITCHFUNC_DREXP_AIL);

    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->drelelbl, COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL, _tr("D/R&Exp -ELE"));
    GUI_CreateTextSelect(&gui->drele, COL2, row, TEXTSELECT_128, NULL, switch_cb2, (void *)(long)SWITCHFUNC_DREXP_ELE);

    row += ROW_SPACE;
    GUI_CreateLabelBox(&gui->drrudlbl, COL1, row, 0, 16, &DEFAULT_FONT, NULL, NULL,  _tr("D/R&Exp -RUD"));
    GUI_CreateTextSelect(&gui->drrud, COL2, row, TEXTSELECT_128, NULL, switch_cb2, (void *)(long)SWITCHFUNC_DREXP_RUD);
}
#endif //HAS_STANDARD_GUI
