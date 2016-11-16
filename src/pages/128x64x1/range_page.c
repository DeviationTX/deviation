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
#include "pages.h"
#include "gui/gui.h"

#include "../common/_range_page.c"

static struct range_obj * const gui = &gui_objs.u.range;

static unsigned action_cb(u32 button, unsigned flags, void *data) {
    (void)data;
    if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
        if (flags & BUTTON_RELEASE) {
            RANGE_test(!mp->testing);
        }
        return 1;
    }
    return default_button_action_cb(button, flags, data);
}

static void _draw_page(int has_pa) {
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_RANGE));

    if (!has_pa) {
        snprintf(tempstring, sizeof(tempstring), _tr("No range test possible."));
    } else {
        PAGE_SetActionCB(action_cb);
        if (mp->testing) {
            snprintf(tempstring, sizeof(tempstring), "%s %s\n%s %s\n%s",
                     _tr("Power reduced to"), RADIO_TX_POWER_VAL[Model.tx_power],
                     _tr("from"), RADIO_TX_POWER_VAL[mp->old_power],
                     _tr("Press ENT to stop test."));
        } else {
            snprintf(tempstring, sizeof(tempstring),
                     _tr("Press ENT to start test."));
        }
    }
    GUI_CreateLabelBox(&gui->label, 0, 15, LCD_WIDTH, LCD_HEIGHT-15, &DEFAULT_FONT,
                       NULL, NULL, tempstring);
}
