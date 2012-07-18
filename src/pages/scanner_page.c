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

#include "target.h"
#include "protocol/interface.h"
#include "misc.h"
#include "pages.h"

#define MIN_RADIOCHANNEL     0x04
#define MAX_RADIOCHANNEL     0x54
static u8 channelnoise[MAX_RADIOCHANNEL - MIN_RADIOCHANNEL];
static guiObject_t *bar[MAX_RADIOCHANNEL - MIN_RADIOCHANNEL];
static u8 channel;
static u8 time_to_scan;

u16 scan_trigger_cb()
{
    time_to_scan = 1;
    return 1250;
}

static s16 show_bar_cb(void *data)
{
    long ch = (long)data;
    return channelnoise[ch];
}

void PAGE_ScannerInit(int page)
{
    u8 i;
    (void)page;
    DEVO_Initialize();  //Switch to DEVO configuration
    CLOCK_StopTimer();
    CYRF_ConfigRxTx(0);
    CYRF_ConfigCRCSeed(0);
    //CYRF_ConfigSOPCode(0);

    GUI_CreateLabel(8, 10, NULL, DEFAULT_FONT, "Scanner");

    time_to_scan = 0;
    channel = MIN_RADIOCHANNEL;
    for(i = 0; i < MAX_RADIOCHANNEL - MIN_RADIOCHANNEL; i++) {
        bar[i] = GUI_CreateBarGraph(i * 4, 40, 4, 192, 0, 0x20, BAR_VERTICAL, show_bar_cb, (void *)((long)i));
        channelnoise[i] = 0x10;
    }
    CLOCK_StartTimer(1250, scan_trigger_cb);
}

void PAGE_ScannerEvent()
{
    u8 dpbuffer[16];
    if(time_to_scan) {
        time_to_scan = 0;
        CYRF_ConfigRFChannel(channel + MIN_RADIOCHANNEL);
    CYRF_ReadRSSI(1);
    CYRF_StartReceive();
    Delay(10);

    CYRF_ReadDataPacket(dpbuffer);
        channelnoise[channel] = CYRF_ReadRSSI(1) & 0x1F;
        GUI_Redraw(bar[channel]);

    //printf("%02X : %d\n",channel,channelnoise[channel]);

        channel++;
        if(channel == MAX_RADIOCHANNEL - MIN_RADIOCHANNEL)
            channel = 0;
    }
}

int PAGE_ScannerCanChange()
{
    return 1;
}
