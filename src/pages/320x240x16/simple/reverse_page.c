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
#include "../../common/simple/_reverse_page.c"

static void toggle_reverse_cb(guiObject_t *obj, void *data)
{
    u8 ch = (long)data;
    if (ch >= NUM_OUT_CHANNELS)
        return;
    Model.limits[ch].flags ^= CH_REVERSE;
}
void PAGE_ReverseInit(int page)
{
    (void)page;
    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_REVERSE), MODELMENU_Show);

    for (long i = 0; i < ENTRIES_PER_PAGE; i++) {
        int row = 40 + 20 * i;
        GUI_CreateLabelBox(30, row, 0, 16, &DEFAULT_FONT, SIMPLEMIX_channelname_cb, NULL, (void *)(i));
        GUI_CreateTextSelect(150, row, TEXTSELECT_128, 0x0000, toggle_reverse_cb, reverse_cb, (void *)(i));
    }
}
