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
#include "protocol/interface.h"
#include "pages.h"
#include "config/model.h"

#if HAS_SCANNER
#include "../common/_scanner_page.c"

static unsigned action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;

    if(flags & BUTTON_RELEASE) {
        if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
                sp->enable ^= 1;
                _scan_enable(sp->enable);
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
                sp->scan_mode ^= 1;
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
                sp->attenuator ^= 1;
        }
    }

    return 1;
}

void _draw_page(u8 enable)
{
    if (enable)
    {
        PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));

        BUTTON_RegisterCallback(&sp->action,
              CHAN_ButtonMask(BUT_ENTER)
              | CHAN_ButtonMask(BUT_LEFT)
              | CHAN_ButtonMask(BUT_RIGHT),
              BUTTON_PRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);
    }
    else
    {
        BUTTON_UnregisterCallback(&sp->action);
    }
}

void _draw_channels()
{
    // draw a line
    int col = (LCD_WIDTH - (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL)) / 2 + sp->channel;
    int height = LCD_HEIGHT - sp->channelnoise[sp->channel] * (LCD_HEIGHT - HEADER_HEIGHT) / 0x1F;

    LCD_DrawFastVLine(col, HEADER_HEIGHT, height + HEADER_HEIGHT, 0);
    LCD_DrawFastVLine(col, height + HEADER_HEIGHT, LCD_HEIGHT, Display.xygraph.grid_color);
}

#endif //HAS_SCANNER
