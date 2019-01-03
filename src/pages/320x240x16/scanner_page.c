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
static s32 show_bar_cb(void *data)
{
    long ch = (long)data;
    return sp->channelnoise[ch];
}

static const char *enablestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp->enable ? _tr("Turn Off") : _tr("Turn On");
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
    return sp->attenuator ? _tr("Att.: -20dB") : _tr("Att.: 0dB");
}

void _draw_page(u8 enable)
{
    enum {
        SCANBARWIDTH   = (LCD_WIDTH / (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1)),
        SCANBARXOFFSET = ((LCD_WIDTH - SCANBARWIDTH * (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1))/2),
        SCANBARHEIGHT  = (LCD_HEIGHT - 78),
    };
    u8 i;
    (void)enable;
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));
    GUI_CreateButton(&gui->enable, LCD_WIDTH/2 - 152, 40, BUTTON_96, enablestr_cb, press_enable_cb, NULL);
    GUI_CreateButton(&gui->scan_mode, LCD_WIDTH/2 - 48, 40, BUTTON_96, modestr_cb, press_mode_cb, NULL);
    GUI_CreateButton(&gui->attenuator, LCD_WIDTH/2 + 56, 40, BUTTON_96, attstr_cb, press_attenuator_cb, NULL);
    for(i = 0; i < (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1); i++) {
        GUI_CreateBarGraph(&gui->bar[i], SCANBARXOFFSET + i * SCANBARWIDTH, 70, SCANBARWIDTH, SCANBARHEIGHT, 2, 31, BAR_VERTICAL, show_bar_cb, (void *)((long)i));
    }
}

void _draw_channels()
{
    GUI_Redraw(&gui->bar[sp->channel]);
}

#endif //HAS_SCANNER
