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
static struct scanner_page * const sp = &pagemem.u.scanner_page;
static struct scanner_obj * const gui = &gui_objs.u.scanner;
static u8 scanState = 0;

u16 scan_cb()
{
    int delay;
    
    if(scanState == 0) {
        if(sp->time_to_scan == 0) {
            CYRF_ConfigRFChannel(sp->channel + MIN_RADIOCHANNEL);
            if(sp->attenuator) {
                CYRF_WriteRegister(CYRF_06_RX_CFG, 0x0A);
            } else {
                CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
            }
            sp->time_to_scan = 1;
        }
        scanState = 1;
        delay = 300; //slow channel require 270usec for synthesizer to settle 
    } else {
        if ( !(CYRF_ReadRegister(CYRF_05_RX_CTRL) & 0x80)) {
            CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80); //Prepare to receive
            Delay(10);
            CYRF_ReadRegister(CYRF_13_RSSI); //dummy read
            Delay(15);
        }
        int rssi = CYRF_ReadRegister(CYRF_13_RSSI) & 0x1F;
        if(sp->scan_mode) {
            sp->channelnoise[sp->channel] = (sp->channelnoise[sp->channel] + rssi) / 2;
        } else {
            if(rssi > sp->channelnoise[sp->channel])
                sp->channelnoise[sp->channel] = rssi;
        }
        scanState++;
        delay = 300;
        if(scanState == 5) {
            scanState = 0;
            delay = 50;
        }
    }
    return delay;
}

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

static void press_enable_cb(guiObject_t *obj, const void *data)
{
    (void)data;
#ifndef MODULAR
    sp->enable ^= 1;
    if (sp->enable) {
        PROTOCOL_DeInit();
        DEVO_Cmds(0);  //Switch to DEVO configuration
        PROTOCOL_SetBindState(0); //Disable binding message
        CLOCK_StopTimer();
        CYRF_SetTxRxMode(RX_EN); //Receive mode
        CLOCK_StartTimer(1250, scan_cb);
    } else {
        PROTOCOL_Init(0);
    }
#endif
    GUI_Redraw(obj);
}

static void press_mode_cb(guiObject_t *obj, const void *data)
{
    (void)data;
#ifndef MODULAR
    sp->scan_mode ^= 1;
#endif
    GUI_Redraw(obj);
}

static void press_attenuator_cb(guiObject_t *obj, const void *data)
{
    (void)data;
#ifndef MODULAR
    sp->attenuator ^= 1;
#endif
    GUI_Redraw(obj);
}

void PAGE_ScannerInit(int page)
{
    enum {
        SCANBARWIDTH   = (LCD_WIDTH / (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1)),
        SCANBARXOFFSET = ((LCD_WIDTH - SCANBARWIDTH * (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1))/2),
        SCANBARHEIGHT  = (LCD_HEIGHT - 78),
    };
    u8 i;
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_SCANNER));
    sp->enable = 0;
    GUI_CreateButton(&gui->enable, LCD_WIDTH/2 - 152, 40, BUTTON_96, enablestr_cb, press_enable_cb, NULL);
    sp->scan_mode = 0;
    GUI_CreateButton(&gui->scan_mode, LCD_WIDTH/2 - 48, 40, BUTTON_96, modestr_cb, press_mode_cb, NULL);
    sp->attenuator = 0;
    GUI_CreateButton(&gui->attenuator, LCD_WIDTH/2 + 56, 40, BUTTON_96, attstr_cb, press_attenuator_cb, NULL);    
    sp->channel = 0;
    sp->time_to_scan = 0;
    for(i = 0; i < (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1); i++) {
        GUI_CreateBarGraph(&gui->bar[i], SCANBARXOFFSET + i * SCANBARWIDTH, 70, SCANBARWIDTH, SCANBARHEIGHT, 2, 31, BAR_VERTICAL, show_bar_cb, (void *)((long)i));
        sp->channelnoise[i] = 0;
    }
}

void PAGE_ScannerEvent()
{
#ifndef MODULAR
    if(! sp->enable)
        return;
    GUI_Redraw(&gui->bar[sp->channel]);
    //printf("%02X : %d\n",sp->channel,sp->channelnoise[sp->channel]);
    sp->channel++;
    if(sp->channel == (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1))
        sp->channel = 0;
    sp->channelnoise[sp->channel] = 0;
    sp->time_to_scan = 0;
#endif
}

void PAGE_ScannerExit()
{
#ifndef MODULAR
    if(sp->enable)
        PROTOCOL_Init(0);
#endif
}

#endif //HAS_SCANNER
