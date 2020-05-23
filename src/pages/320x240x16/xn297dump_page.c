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

#if SUPPORT_XN297DUMP

static struct xn297dump_obj * const gui = &gui_objs.u.xn297dump;

#include "../common/_xn297dump_page.c"

static const char *mode_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (Model.protocol != PROTOCOL_XN297DUMP)
        return "---";
    xn297dump.mode = GUI_TextSelectHelper(xn297dump.mode, 0, 3, dir, 1, 1, NULL);
    _dump_enable(xn297dump.mode);

    if (xn297dump.mode != XN297DUMP_INTERVAL)
        xn297dump.interval = 0;
    const void * modelbl[4] = { "Off", "Manual", "Scan", "Interval" };
    return modelbl[xn297dump.mode];
}

static const char *channel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    xn297dump.channel = GUI_TextSelectHelper(xn297dump.channel, 0, MAX_RF_CHANNEL, dir, 1, 10, NULL);
    snprintf(tempstring, sizeof(tempstring), "Channel %d", xn297dump.channel);
    return tempstring;
}

static const char *pktlen_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    xn297dump.pkt_len = GUI_TextSelectHelper(xn297dump.pkt_len, 1, MAX_PAYLOAD, dir, 1, 5, NULL);
    snprintf(tempstring, sizeof(tempstring), "Length %d", xn297dump.pkt_len);
    return tempstring;
}

static void _draw_page()
{
    PAGE_ShowHeader(PAGE_GetName(PAGEID_XN297DUMP));
    GUI_CreateTextSelect(&gui->mode, LCD_WIDTH/2 - 152, 44, TEXTSELECT_96, NULL, mode_cb, NULL);
    GUI_CreateTextSelect(&gui->channel, LCD_WIDTH/2 - 152, 64, TEXTSELECT_128, scan_cb, channel_cb, NULL);
    GUI_CreateTextSelect(&gui->pkt_len,  LCD_WIDTH/2, 64, TEXTSELECT_128, NULL, pktlen_cb, NULL);
    for (int i = 0; i < 4; i++) {
        GUI_CreateLabelBox(&gui->packetdata[i], LCD_WIDTH/2 - LCD_WIDTH/4, 100 + 20 * i, LCD_WIDTH/2, 18, &NORMALBOX_FONT, packetdata_cb, NULL, (void *)(uintptr_t)i);
    }
    GUI_CreateLabelBox(&gui->status, LCD_WIDTH/2 - LCD_WIDTH/3, LCD_HEIGHT - 30, LCD_WIDTH/3*2, 18, &NORMALBOX_FONT, status_cb, NULL, NULL);
}

#endif  // SUPPORT_XN297DUMP
