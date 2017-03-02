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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "../pages.h"
#include "gui/gui.h"
#include "config/model.h"
#include "standard.h"

enum {
    FIELD_X        = 77,
    FIELD_WIDTH    = 46,
    LABEL_X        = 0,
    LABEL_WIDTH    = FIELD_X - LABEL_X,
};
#endif //OVERRIDE_PLACEMENT

#if HAS_STANDARD_GUI
#include "../../common/standard/_switchassign_page.c"

static unsigned _action_cb(u32 button, unsigned flags, void *data);

static const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    FunctionSwitch i = (long)data;
    switch (i) {
    case SWITCHFUNC_FLYMODE:
        tempstring_cpy(_tr("Fly mode"));
        break;
    case SWITCHFUNC_HOLD:
        tempstring_cpy(_tr("Thr hold"));
        break;
    case SWITCHFUNC_GYROSENSE:
        tempstring_cpy(_tr("Gyro sense"));
        break;
    case SWITCHFUNC_DREXP_AIL:
        tempstring_cpy(_tr("D/R&Exp -AIL"));
        break;
    case SWITCHFUNC_DREXP_ELE:
        tempstring_cpy(_tr("D/R&Exp -ELE"));
        break;
    case SWITCHFUNC_DREXP_RUD:
    default:
        tempstring_cpy(_tr("D/R&Exp -RUD"));
        break;
    }
    return tempstring;
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    GUI_CreateLabelBox(&gui->name[relrow], LABEL_X, y, LABEL_WIDTH, LINE_HEIGHT, &LABEL_FONT, label_cb, NULL, (void *)(long)absrow);
    GUI_CreateTextSelectPlate(&gui->value[relrow], FIELD_X, y,
        FIELD_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, switch_cb, (void *)(long)absrow);
    return 1;
}

void PAGE_SwitchAssignInit(int page)
{
    (void)page;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    refresh_switches();

    PAGE_ShowHeader(_tr("Press ENT to change"));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                     LINE_SPACE, SWITCHFUNC_LAST, row_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
        MUSIC_Play(MUSIC_SAVING);
        save_changes();
        return 1;
    }
    return default_button_action_cb(button, flags, data);
}
#endif //HAS_STANDARD_GUI
