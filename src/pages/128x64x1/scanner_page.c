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

static struct scanner_obj * const gui = &gui_objs.u.scanner;
static const char *enablestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp->enable ? _tr("On") : _tr("Off");
}

static const char *modestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp->scan_mode ? _tr("Average") : _tr("Peak");
}

static const char *attstr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp->attenuator ? _tr("-20dB") : _tr("0dB");
}

static void press_enable_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    sp->enable ^= 1;
    _scan_enable(sp->enable);
    GUI_Redraw(obj);
}

static void press_mode_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    sp->scan_mode ^= 1;
    GUI_Redraw(obj);
}

static void press_attenuator_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    sp->attenuator ^= 1;
    GUI_Redraw(obj);
}


void _draw_page(u8 enable)
{
    (void)enable;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));
    GUI_CreateButtonPlateText(&gui->enable, 0, HEADER_HEIGHT, 40, LINE_HEIGHT, &BUTTON_FONT, enablestr_cb, press_enable_cb, NULL);
    GUI_CreateButtonPlateText(&gui->scan_mode, LCD_WIDTH/2 - 20, HEADER_HEIGHT, 40, LINE_HEIGHT, &BUTTON_FONT, modestr_cb, press_mode_cb, NULL);
    GUI_CreateButtonPlateText(&gui->attenuator, LCD_WIDTH - 40, HEADER_HEIGHT, 40, LINE_HEIGHT, &BUTTON_FONT, attstr_cb, press_attenuator_cb, NULL);
}

void _draw_channels()
{
    const unsigned offset = HEADER_HEIGHT + LINE_HEIGHT;
    // draw a line
    int col = (LCD_WIDTH - (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL)) / 2 + sp->channel;
    int height = sp->channelnoise[sp->channel] * (LCD_HEIGHT - offset) / 0x1F;

    LCD_DrawFastVLine(col, offset, LCD_HEIGHT - height, 0);
    LCD_DrawFastVLine(col, LCD_HEIGHT - height, LCD_HEIGHT, Display.xygraph.grid_color);
}

#endif //HAS_SCANNER
