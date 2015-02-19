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
#include "../../common/standard/_throhold_page.c"
static unsigned _action_cb(u32 button, unsigned flags, void *data);

static s8 current_selected = 0;
void PAGE_ThroHoldInit(int page)
{
    (void)page;
    //if (page < 0 && current_selected > 0) // enter this page from childen page , so we need to get its previous mp->current_selected item
    //    page = current_selected;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    current_selected = 0;

    PAGE_ShowHeader(_tr("Throttle hold"));

    u8 y = HEADER_HEIGHT;
    GUI_CreateLabelBox(&gui->enlbl, 0, y, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Thr hold"));
    u8 w = 40;
    GUI_CreateTextSelectPlate(&gui->en, 75, y, w, LINE_HEIGHT, &DEFAULT_FONT, NULL, throhold_cb,  NULL);

    y += LINE_SPACE;
    GUI_CreateLabelBox(&gui->valuelbl, 0, y, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("Hold position"));
    GUI_CreateTextSelectPlate(&gui->value, 75, y, w, LINE_HEIGHT, &DEFAULT_FONT, NULL, holdpostion_cb,  NULL);

    GUI_Select1stSelectableObj();

}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
#endif //HAS_STANDARD_GUI
