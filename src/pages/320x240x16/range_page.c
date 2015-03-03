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

static void start_test(struct guiObject *gui, const void *data) {
    (void) gui;
    RANGE_test(!data);
}

static const char *startstop_dlg(struct guiObject *gui, const void *data) {
    (void) gui;
    return data ? _tr("Stop test") : _tr("Start test");
}
      
#define INFO_X (LCD_WIDTH/2-126)
#define INFO_Y 70

static void _draw_page(int has_pa) {
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_RANGE));
    if (!has_pa) {
        snprintf(tempstring, sizeof(tempstring), _tr("No range test possible."));
        GUI_CreateLabelBox(&gui->info, INFO_X, INFO_Y, 0, 0, &DEFAULT_FONT,
                           NULL, NULL, tempstring);
    } else {
        GUI_CreateButton(&gui->button, INFO_X, 40, DIALOG_BUTTON,
                         startstop_dlg, 0x0000, start_test,
                         (void *)(long)mp->testing);
        if (mp->testing) {
            snprintf(tempstring, sizeof(tempstring), "%s %s %s %s.",
                     _tr("Power reduced to"), RADIO_TX_POWER_VAL[Model.tx_power],
                     _tr("from"), RADIO_TX_POWER_VAL[mp->old_power]);
            GUI_CreateLabelBox(&gui->info, INFO_X, INFO_Y, 0, 0, &DEFAULT_FONT,
                               NULL, NULL, tempstring);
        }
    }
}
