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

#if SUPPORT_SCANNER
#include "../common/_scanner_page.c"

static struct scanner_obj * const gui = &gui_objs.u.scanner;
static const char *enablestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp->enable ? _tr("On") : _tr("Off");
}

static void _draw_page(u8 enable)
{
    (void)enable;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));
    GUI_CreateButtonPlateText(&gui->enable, 0, HEADER_HEIGHT, 40, LINE_HEIGHT, &BUTTON_FONT, enablestr_cb, press_enable_cb, NULL);
    GUI_CreateTextSelectPlate(&gui->averaging, LCD_WIDTH/2 - 23, HEADER_HEIGHT, 46, LINE_HEIGHT, &TEXTSEL_FONT, NULL, average_cb, NULL);
    GUI_CreateTextSelectPlate(&gui->attenuator, LCD_WIDTH - 40, HEADER_HEIGHT, 40, LINE_HEIGHT, &TEXTSEL_FONT, NULL, attenuator_cb, NULL);
}

void _draw_channels()
{
    const unsigned offset = HEADER_HEIGHT + LINE_HEIGHT;
    int col, height;
    int num_chan = sp->chan_max - sp->chan_min;

    // draw rssi values
    for (int i = 0; i < num_chan; i++) {
        col = (LCD_WIDTH - (num_chan)) / 2 + i;
        height = sp->rssi[i] * (LCD_HEIGHT - offset) / 0x1F;

        LCD_DrawFastVLine(col, offset, LCD_HEIGHT - offset - height, 0);
        LCD_DrawFastVLine(col, LCD_HEIGHT - height, height, Display.xygraph.grid_color);
    }
}

#endif  // SUPPORT_SCANNER
