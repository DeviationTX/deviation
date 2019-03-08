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

static struct scanner_page * const sp = &pagemem.u.scanner_page;

static void _draw_page(u8 enable);
static void _draw_channels(void);
static u16 scan_cb();

#ifdef ENABLE_MODULAR
#error "Not supported in MODULAR build"
#endif

// The high level interface to do the scan
static void _scan_enable(int enable)
{
    if (enable) {
        PROTOCOL_DeInit();
        DEVO_Cmds(0);  //Switch to DEVO configuration
        PROTOCOL_SetBindState(0); //Disable binding message
        CLOCK_StopTimer();
        CYRF_SetTxRxMode(RX_EN); //Receive mode
        CLOCK_StartTimer(1250, scan_cb);
    } else {
        PROTOCOL_Init(0);
    }
}

static void _scan_next()
{
    CYRF_ConfigRFChannel(sp->channel + MIN_RADIOCHANNEL);
    if(sp->attenuator) {
        CYRF_WriteRegister(CYRF_06_RX_CFG, 0x0A);
    } else {
        CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
    }
}

static int _scan_rssi()
{
    if ( !(CYRF_ReadRegister(CYRF_05_RX_CTRL) & 0x80)) {
        CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80); //Prepare to receive
        Delay(10);
        CYRF_ReadRegister(CYRF_13_RSSI); //dummy read
        Delay(15);
    }
#ifdef EMULATOR
    return rand32() % 0x1F;
#else
    return CYRF_ReadRegister(CYRF_13_RSSI) & 0x1F;
#endif
}

u16 scan_cb()
{
    int delay;
    if(sp->scanState == 0) {
        if(sp->time_to_scan == 0) {
            _scan_next();
            sp->time_to_scan = 1;
        }
        sp->scanState = 1;
        delay = 300; //slow channel require 270usec for synthesizer to settle
    } else {
        int rssi = _scan_rssi();
        if(sp->scan_mode) {
            sp->channelnoise[sp->channel] = (sp->channelnoise[sp->channel] + rssi) / 2;
        } else {
            if(rssi > sp->channelnoise[sp->channel])
                sp->channelnoise[sp->channel] = rssi;
        }
        sp->scanState++;
        delay = 300;
        if(sp->scanState == 5) {
            sp->scanState = 0;
            delay = 50;
        }
    }
    return delay;
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

void PAGE_ScannerInit(int page)
{
    (void)page;
    memset(sp, 0, sizeof(struct scanner_page));

    PAGE_SetModal(0);
    _draw_page(1);
}

void PAGE_ScannerEvent()
{
    if(! sp->enable)
        return;

    // draw the channels
    _draw_channels();

    sp->channel++;
    if(sp->channel == (MAX_RADIOCHANNEL - MIN_RADIOCHANNEL + 1))
        sp->channel = 0;
    sp->channelnoise[sp->channel] = 0;
    sp->time_to_scan = 0;
}

void PAGE_ScannerExit()
{
    if (sp->enable)
        _scan_enable(0);
}
