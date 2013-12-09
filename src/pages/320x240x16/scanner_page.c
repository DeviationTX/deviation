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

#ifdef ENABLE_SCANNER
#define sp (pagemem.u.scanner_page)
static struct scanner_obj * const gui = &gui_objs.u.scanner;

u16 scan_trigger_cb()
{
    sp.time_to_scan = 1;
    return 1250;
}

static s16 show_bar_cb(void *data)
{
    long ch = (long)data;
    return sp.channelnoise[ch];
}

static const char *enablestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return sp.enable ? _tr("Turn Off") : _tr("Turn On");
}
static void press_cb(guiObject_t *obj, const void *data)
{
    (void)data;
#ifndef MODULAR
    sp.enable ^= 1;
    if (sp.enable) {
        PROTOCOL_DeInit();
        DEVO_Cmds(0);  //Switch to DEVO configuration
        PROTOCOL_SetBindState(0); //Disable binding message
        CLOCK_StopTimer();
        CYRF_ConfigRxTx(0);
        CYRF_ConfigCRCSeed(0);
        CLOCK_StartTimer(1250, scan_trigger_cb);
        //CYRF_ConfigSOPCode(0);
    } else {
        PROTOCOL_Init(0);
    }
#endif
    GUI_Redraw(obj);
}

void PAGE_ScannerInit(int page)
{
    u8 i;
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));
    sp.enable = 0;
    GUI_CreateButton(&gui->enable, LCD_WIDTH/2-48, 40, BUTTON_96, enablestr_cb, 0x0000, press_cb, NULL);

    sp.time_to_scan = 0;
    sp.channel = MIN_RADIOCHANNEL;
    #define SCANBARWIDTH   (LCD_WIDTH / (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL))
    #define SCANBARXOFFSET ((LCD_WIDTH - SCANBARWIDTH * (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL))/2)
    #define SCANBARHEIGHT  (LCD_HEIGHT - 78)
    for(i = 0; i < MAX_RADIOCHANNEL - MIN_RADIOCHANNEL; i++) {
        GUI_CreateBarGraph(&gui->bar[i], SCANBARXOFFSET + i * SCANBARWIDTH, 70, SCANBARWIDTH, SCANBARHEIGHT, 0, 0x20, BAR_VERTICAL, show_bar_cb, (void *)((long)i));
        sp.channelnoise[i] = 0x10;
    }
}

void PAGE_ScannerEvent()
{
#ifndef MODULAR
    u8 dpbuffer[16];
    if(! sp.enable)
        return;
    if(sp.time_to_scan) {
        sp.time_to_scan = 0;
        CYRF_ConfigRFChannel(sp.channel + MIN_RADIOCHANNEL);
    CYRF_ReadRSSI(1);
    CYRF_StartReceive();
    Delay(10);

    CYRF_ReadDataPacket(dpbuffer);
        sp.channelnoise[sp.channel] = CYRF_ReadRSSI(1) & 0x1F;
        GUI_Redraw(&gui->bar[sp.channel]);

    //printf("%02X : %d\n",sp.channel,sp.channelnoise[sp.channel]);

        sp.channel++;
        if(sp.channel == MAX_RADIOCHANNEL - MIN_RADIOCHANNEL)
            sp.channel = 0;
    }
#endif
}

void PAGE_ScannerExit()
{
#ifndef MODULAR
    if(sp.enable)
        PROTOCOL_Init(0);
#endif
}

#endif
