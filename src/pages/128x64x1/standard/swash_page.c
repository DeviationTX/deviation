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

#if HAS_STANDARD_GUI
#include "../../common/standard/_swash_page.c"

static unsigned _action_cb(u32 button, unsigned flags, void *data);

void PAGE_SwashInit(int page)
{
    (void)page;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    get_swash();

    u8 w = 60;
    u8 x = 63;
    PAGE_ShowHeader(_tr("SwashType"));
    GUI_CreateTextSelectPlate(&gui->type, x-3, 0, w + 8, LINE_HEIGHT - 1, &DEFAULT_FONT, NULL, swash_val_cb, NULL); // FIXME: need a special value for header button/textsels

    u8 row = LINE_SPACE;
    GUI_CreateLabelBox(&gui->lbl[0], 0, row, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("ELE Mix"));
    GUI_CreateTextSelectPlate(&gui->mix[0], x, row, w, LINE_HEIGHT, &DEFAULT_FONT, NULL, swashmix_val_cb, (void *)1);

    row += LINE_SPACE;
    GUI_CreateLabelBox(&gui->lbl[1], 0, row, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("AIL Mix"));
    GUI_CreateTextSelectPlate(&gui->mix[1], x, row, w, LINE_HEIGHT, &DEFAULT_FONT, NULL, swashmix_val_cb, (void *)0);

    row += LINE_SPACE;
    GUI_CreateLabelBox(&gui->lbl[2], 0, row, 0, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr("PIT Mix"));
    GUI_CreateTextSelectPlate(&gui->mix[2], x, row, w, LINE_HEIGHT, &DEFAULT_FONT, NULL, swashmix_val_cb, (void *)2);

    update_swashmixes();
    GUI_Select1stSelectableObj();
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    //u8 total_items = 2;
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
