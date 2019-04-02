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

static void _draw_page(u8 enable);

static void _dump_enable(int enable)
{
    if (enable) {
        PROTOCOL_DeInit();
        xp->model_protocol = Model.protocol;  // Save protocol used in current Model file
        Model.protocol = PROTOCOL_XN297DUMP;
        PROTOCOL_Init(1);  // Switch to scanner configuration and ignore safety
        PROTOCOL_SetBindState(0);  // Disable binding message
    } else {
        PROTOCOL_DeInit();
        Model.protocol = xp->model_protocol;
        PROTOCOL_Init(0);
    }
}

static void press_enable_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    xp->enable ^= 1;
    _dump_enable(xp->enable);
    GUI_Redraw(obj);
}

static const char *packetdata_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 first_packet = (long)data * 8;
    int idx = 0;
    for (unsigned i = 0; i < 8; i++) {
        snprintf(&tempstring[idx], sizeof(tempstring) - idx, "%02X ", xn297dump.packet[i + first_packet]);
        idx += 3;
    }

    return tempstring;
}

void PAGE_XN297DumpInit(int page)
{
    (void)page;
    memset(xp, 0, sizeof(struct xn297dump_page));
    xn297dump.pkt_len = 32;
    xn297dump.channel = 0;
    PAGE_SetModal(0);
    _draw_page(1);
}

void PAGE_XN297DumpEvent()
{
    if(! xp->enable)
        return;
    for (int i = 0; i < 4; i++) {
        GUI_Redraw(&gui->packetdata[i]);
    }
}

void PAGE_XN297DumpExit()
{
    if (xp->enable)
        _dump_enable(0);
}
