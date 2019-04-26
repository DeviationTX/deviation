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

#include "protocol/interface.h"
#include "rftools.h"

static struct xn297dump_page * const xp = &pagemem.u.xn297dump_page;

static void _draw_page();

static void _dump_enable(int enable)
{
    if (enable) {
        PROTOCOL_Init(1);
        PROTOCOL_SetBindState(0);  // Disable binding message
    } else {
        PROTOCOL_DeInit();
    }
}

static void _write_dump()
{
    for (unsigned int i = 0 ; i < xn297dump.pkt_len ; ++i) {
        if (xp->last_packet[i] != xn297dump.packet[i]) {
            memcpy(xp->last_packet, xn297dump.packet, xn297dump.pkt_len);
            RFTOOLS_DumpXN297Packet(&xp->last_packet);
        }
    }
}

static const char *packetdata_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 first_packet = (uintptr_t)data * 8;
    int idx = 0;
    for (unsigned i = 0; i < 8; i++) {
        snprintf(&tempstring[idx], sizeof(tempstring) - idx, "%02X ", xn297dump.packet[i + first_packet]);
        idx += 3;
    }

    return tempstring;
}

static void scan_cb(guiObject_t *obj, void *data)
{
    (void)data;
    (void)obj;
    xn297dump.scan ^= 1;
    if (xn297dump.mode == XN297DUMP_MANUAL)
        RFTOOLS_InitDumpLog(xn297dump.scan);
    if (xn297dump.mode == XN297DUMP_SCAN)
        xn297dump.channel++;
}

static const char *status_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if (Model.protocol != PROTOCOL_XN297DUMP)
        return _tr_noop("Set protocol to XN297Dump");
    if (xn297dump.scan) {
        if (xn297dump.mode == XN297DUMP_SCAN)
            return _tr_noop("Scanning channels and length...");
        else if (xn297dump.mode == XN297DUMP_INTERVAL)
            return _tr_noop("Measuring packet intervals...");
        else if (xn297dump.mode == XN297DUMP_MANUAL)
            return _tr_noop("Recording packets...");
    }
    if (xn297dump.interval) {
        snprintf(tempstring, sizeof(tempstring), "Packet interval %d usec", xn297dump.interval);
        return tempstring;
    }
    return xn297dump.crc_valid ? _tr_noop("Valid CRC found!") : _tr_noop("CRC invalid");
}

void PAGE_XN297DumpInit(int page)
{
    (void)page;
    memset(xp, 0, sizeof(struct xn297dump_page));
    xn297dump.crc_valid = 0;
    xn297dump.pkt_len = MAX_PAYLOAD;
    xn297dump.mode = 0;
    xn297dump.scan = 0;
    PAGE_SetModal(0);
    _draw_page();
}

void PAGE_XN297DumpEvent()
{
    switch (xn297dump.mode) {
        case XN297DUMP_OFF:
            return;
        case XN297DUMP_MANUAL:
            if (xn297dump.scan) {
                _write_dump();
            }
            break;
        case XN297DUMP_SCAN:
            GUI_Redraw(&gui->channel);
            GUI_Redraw(&gui->pkt_len);
            break;
        case XN297DUMP_INTERVAL:
            if (xn297dump.scan) {
                GUI_Redraw(&gui->status);  // only update status before measuring interval to avoid ugly display
                return;
            }
    }
    for (int i = 0; i < 4; i++) {
        GUI_Redraw(&gui->packetdata[i]);
    }
    GUI_Redraw(&gui->status);
}

void PAGE_XN297DumpExit()
{
    _dump_enable(0);
}
