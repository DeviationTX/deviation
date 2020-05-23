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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "config/model.h"

enum {
    MODE_X        = 0,
    MODE_Y        = 13,
    MODE_WIDTH    = 40,
    CHANNEL_X     = LCD_WIDTH/2 - 23,
    CHANNEL_Y     = 13,
    CHANNEL_WIDTH = 42,
    LENGTH_X      = LCD_WIDTH - 44,
    LENGTH_Y      = 13,
    LENGTH_WIDTH  = 44,
    PACKET_X      = 0,
    PACKET_Y      = 26,
    PACKET_WIDTH  = LCD_WIDTH,
    PACKET_HEIGHT = 7,
    STATUS_Y      = 56,
};
#endif  // OVERRIDE_PLACEMENT

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
    const void * modelbl[4] = { "Off", "Man", "Scan", "Intvl" };
    return modelbl[xn297dump.mode];
}

static const char *channel_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    xn297dump.channel = GUI_TextSelectHelper(xn297dump.channel, 0, MAX_RF_CHANNEL, dir, 1, 10, NULL);
    snprintf(tempstring, sizeof(tempstring), "Ch %d", xn297dump.channel);
    return tempstring;
}

static const char *pktlen_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    xn297dump.pkt_len = GUI_TextSelectHelper(xn297dump.pkt_len, 1, MAX_PAYLOAD, dir, 1, 5, NULL);
    snprintf(tempstring, sizeof(tempstring), "Len %d", xn297dump.pkt_len);
    return tempstring;
}

static void _draw_page()
{
    PAGE_ShowHeader(PAGE_GetName(PAGEID_XN297DUMP));
    GUI_CreateTextSelectPlate(&gui->mode, MODE_X, MODE_Y, MODE_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, scan_cb, mode_cb, NULL);
    GUI_CreateTextSelectPlate(&gui->channel, CHANNEL_X, CHANNEL_Y, CHANNEL_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, channel_cb, NULL);
    GUI_CreateTextSelectPlate(&gui->pkt_len, LENGTH_X, LENGTH_Y, LENGTH_WIDTH, LINE_HEIGHT, &TEXTSEL_FONT, NULL, pktlen_cb, NULL);
    for (int i = 0; i < 4; i++) {
        GUI_CreateLabelBox(&gui->packetdata[i], PACKET_X, PACKET_Y + (LINE_HEIGHT / 2 + 1) * i, LCD_WIDTH, PACKET_HEIGHT, &TINY_FONT, packetdata_cb, NULL, (void *)(uintptr_t)i);
    }
    GUI_CreateLabelBox(&gui->status, 0, STATUS_Y, LCD_WIDTH, 7, &TINY_FONT, status_cb, NULL, NULL);
}

#endif  // SUPPORT_XN297DUMP
