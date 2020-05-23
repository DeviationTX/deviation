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
static const char *enablestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp->enable ? _tr("On") : _tr("Off");
}

static const char *average_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    Scanner.averaging = GUI_TextSelectHelper(Scanner.averaging, 1, 8192, dir, 1, 50, NULL);
    switch (sp->mode) {
        case PEAK_MODE:
            snprintf(tempstring, sizeof(tempstring), "Pk %d", Scanner.averaging); break;
        case AVERAGE_MODE:
            snprintf(tempstring, sizeof(tempstring), "Av %d", Scanner.averaging); break;
        case PEAK_HOLD_AVERAGE_MODE:
            snprintf(tempstring, sizeof(tempstring), "AP %d", Scanner.averaging); break;
        case LAST_MODE:
            break;
    }

    memset(Scanner.rssi, 0, sizeof(Scanner.rssi));  // clear old rssi values when changing mode
    memset(Scanner.rssi_peak, 0, sizeof(Scanner.rssi_peak));  // clear old rssi peak values when changing mode
    return tempstring;
}

static void mode_cb(guiObject_t *obj, void *data) {
    (void) obj;
    (void) data;
    sp->mode++;
    if (sp->mode == LAST_MODE)
        sp->mode = PEAK_MODE;
}

static void _draw_page(u8 enable)
{
    (void)enable;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));
    GUI_CreateButtonPlateText(&gui->enable, 0, HEADER_HEIGHT, 40, LINE_HEIGHT, &BUTTON_FONT, enablestr_cb, press_enable_cb, NULL);
    GUI_CreateTextSelectPlate(&gui->averaging, LCD_WIDTH/2 - 23, HEADER_HEIGHT, 46, LINE_HEIGHT, &TEXTSEL_FONT, mode_cb, average_cb, NULL);
    GUI_CreateTextSelectPlate(&gui->attenuator, LCD_WIDTH - 40, HEADER_HEIGHT, 40, LINE_HEIGHT, &TEXTSEL_FONT, NULL, attenuator_cb, NULL);
}

void _draw_channels()
{
    const unsigned offset = HEADER_HEIGHT + LINE_HEIGHT;
    int col, height;

    // draw rssi values
    for (int i = 0; i < Scanner.chan_max - Scanner.chan_min; i++) {
        col = (LCD_WIDTH - (Scanner.chan_max - Scanner.chan_min)) / 2 + i;
        if (sp->mode == PEAK_MODE) {
            height = Scanner.rssi_peak[i] * (LCD_HEIGHT - offset) / 0x1F;
        } else {
            height = Scanner.rssi[i] * (LCD_HEIGHT - offset) / 0x1F;
        }
        LCD_DrawFastVLine(col, offset, LCD_HEIGHT - offset - height, 0);
        LCD_DrawFastVLine(col, LCD_HEIGHT - height, height, 1);

        if (sp->mode == PEAK_HOLD_AVERAGE_MODE) {
            height = Scanner.rssi_peak[i] * (LCD_HEIGHT - offset) / 0x1F;
            LCD_DrawPixelXY(col, LCD_HEIGHT - height, 1);
        }
    }
}

#endif  // SUPPORT_SCANNER
