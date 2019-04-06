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

static void setup_module(int enable)
{
    if (enable) {
        xp->model_protocol = Model.protocol;
        Model.protocol = PROTOCOL_XN297DUMP;
    } else {
        PROTOCOL_DeInit();
        Model.protocol = xp->model_protocol;
        PROTOCOL_Init(0);
    }
}

static void _dump_enable(int enable)
{
    if (enable) {
        PROTOCOL_Init(1);
        PROTOCOL_SetBindState(0);  // Disable binding message
    } else {
        PROTOCOL_DeInit();
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
    if (xn297dump.scan == XN297DUMP_SCAN_OFF) {
        xn297dump.channel++;
        xn297dump.crc_valid = 0;
        xn297dump.scan = XN297DUMP_SCAN_ON;
    } else {
        xn297dump.scan = XN297DUMP_SCAN_OFF;
    }
    GUI_Redraw(obj);
}

static const char *status_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    if (!PROTOCOL_HasModule(PROTOCOL_XN297DUMP))
        return _tr_noop("Module NRF24L01 not available");
    if (xn297dump.scan) {
        return _tr_noop("Scanning channels and length...");
    }
    return xn297dump.crc_valid ? _tr_noop("Valid CRC found!") : _tr_noop("CRC invalid");
}

void PAGE_XN297DumpInit(int page)
{
    (void)page;
    memset(xp, 0, sizeof(struct xn297dump_page));
    setup_module(1);
    xn297dump.crc_valid = 0;
    xn297dump.pkt_len = 32;  // maximum payload length for nrf24l01
    xn297dump.scan = 0;
    PAGE_SetModal(0);
    _draw_page();
}

void PAGE_XN297DumpEvent()
{
    if (!xn297dump.mode)
        return;
    for (int i = 0; i < 4; i++) {
        GUI_Redraw(&gui->packetdata[i]);
    }
    GUI_Redraw(&gui->status);
    if (xn297dump.scan) {
        GUI_Redraw(&gui->channel);
        GUI_Redraw(&gui->pkt_len);
    }
    if (xn297dump.scan == XN297DUMP_SCAN_SUCCESS)
        xn297dump.scan = XN297DUMP_SCAN_OFF;
}

void PAGE_XN297DumpExit()
{
    setup_module(0);
}
