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
#include "config/model.h"

#if SUPPORT_SCANNER
#include "../common/_scanner_page.c"

static struct scanner_obj * const gui = &gui_objs.u.scanner;
static s32 show_bar_cb(void *data)
{
    long ch = (long)data;
    return Scanner.rssi[ch];
}

static const char *enablestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp->enable ? _tr("Turn Off") : _tr("Turn On");
}

static void _draw_page(u8 enable)
{
    (void)enable;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));
    GUI_CreateButton(&gui->enable, LCD_WIDTH/2 - 152, 40, BUTTON_96, enablestr_cb, press_enable_cb, NULL);
    GUI_CreateTextSelect(&gui->averaging, LCD_WIDTH/2 - 48, 44, TEXTSELECT_96, NULL, average_cb, NULL);
    GUI_CreateTextSelect(&gui->attenuator, LCD_WIDTH/2 + 56, 44, TEXTSELECT_96, NULL, attenuator_cb, NULL);
}

void _draw_channels()
{
    if (!sp->bars_valid) {
        const unsigned int height = LCD_HEIGHT - 78;
        int width = LCD_WIDTH / (Scanner.chan_max - Scanner.chan_min + 1);
        int xoffset = (LCD_WIDTH - width * ((Scanner.chan_max - Scanner.chan_min) + 1))/2;
        for (int i = 0; i < (Scanner.chan_max - Scanner.chan_min); i++) {
            GUI_CreateBarGraph(&gui->bar[i], xoffset + i * width, 70, width, height, 2, 31, BAR_VERTICAL, show_bar_cb, (void *)(uintptr_t)i);
        }
        sp->bars_valid = 1;
    }
    for (int i = 0; i < (Scanner.chan_max - Scanner.chan_min); i++)
        GUI_Redraw(&gui->bar[i]);
}

#endif  // SUPPORT_SCANNER
