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
#include "interface.h"
#include "pages/128x64x1/pages.h"

#ifdef PROTO_HAS_CYRF6936

#define MIN_RADIOCHANNEL    0x00
#define MAX_RADIOCHANNEL    0x62
#define CHANNEL_LOCK_TIME   300  // slow channel requires 270 usec for synthesizer to settle
#define INTERNAL_AVERAGE         3
#define AVERAGE_INTVL       50

static struct scanner_page * const sp = &pagemem.u.scanner_page;
static int averages;
static long rssi_sum;

enum ScanStates {
    SCAN_CHANNEL_CHANGE = 0,
    SCAN_GET_RSSI = 1,
};

static void cyrf_init()
{
    /* Initialize CYRF chip */
    CYRF_WriteRegister(CYRF_1D_MODE_OVERRIDE, 0x38);   // FRC SEN (forces the synthesizer to start) + FRC AWAKE (force the oscillator to keep running at all times)
    CYRF_WriteRegister(CYRF_03_TX_CFG, 0x08 | 7);      // Data Code Length = 32 chip codes + Data Mode = 8DR Mode + max-power(+4 dBm)
    CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);          // LNA + FAST TURN EN + RXOW EN, enable low noise amplifier, fast turning, overwrite enable
    CYRF_WriteRegister(CYRF_0B_PWR_CTRL, 0x00);        // Reset power control
    CYRF_WriteRegister(CYRF_10_FRAMING_CFG, 0xA4);     // SOP EN + SOP LEN = 32 chips + LEN EN + SOP TH = 04h
    CYRF_WriteRegister(CYRF_11_DATA32_THOLD, 0x05);    // TH32 = 0x05
    CYRF_WriteRegister(CYRF_12_DATA64_THOLD, 0x0E);    // TH64 = 0Eh, set pn correlation threshold
    CYRF_WriteRegister(CYRF_1B_TX_OFFSET_LSB, 0x55);   // STRIM LSB = 0x55, typical configuration
    CYRF_WriteRegister(CYRF_1C_TX_OFFSET_MSB, 0x05);   // STRIM MSB = 0x05, typical configuration
    CYRF_WriteRegister(CYRF_32_AUTO_CAL_TIME, 0x3C);   // AUTO_CAL_TIME = 3Ch, typical configuration
    CYRF_WriteRegister(CYRF_35_AUTOCAL_OFFSET, 0x14);  // AUTO_CAL_OFFSET = 14h, typical configuration
    CYRF_WriteRegister(CYRF_39_ANALOG_CTRL, 0x01);     // ALL SLOW
    CYRF_WriteRegister(CYRF_1E_RX_OVERRIDE, 0x10);     // FRC RXDR (Force Receive Data Rate)
    CYRF_WriteRegister(CYRF_1F_TX_OVERRIDE, 0x00);     // Reset TX overrides
    CYRF_WriteRegister(CYRF_01_TX_LENGTH, 0x10);       // TX Length = 16 byte packet
    CYRF_WriteRegister(CYRF_27_CLK_OVERRIDE, 0x02);    // RXF, force receive clock
    CYRF_WriteRegister(CYRF_28_CLK_EN, 0x02);          // RXF, force receive clock enable
}

static void _scan_next()
{
    CYRF_ConfigRFChannel(sp->channel + sp->chan_min);
    if (sp->attenuator) {
        CYRF_WriteRegister(CYRF_06_RX_CFG, 0x0A);
    } else {
        CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A);
    }
}

static int _scan_rssi()
{
    if ( !(CYRF_ReadRegister(CYRF_05_RX_CTRL) & 0x80) ) {
        CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80);  // Prepare to receive
        Delay(10);
        CYRF_ReadRegister(CYRF_13_RSSI);  // dummy read
        Delay(15);
    }
#ifdef EMULATOR
    return rand32() % 0x1F;
#else
    return CYRF_ReadRegister(CYRF_13_RSSI) & 0x1F;
#endif
}

static u16 scan_cb()
{
    int rssi_value;
    switch (sp->scanState) {
        case SCAN_CHANNEL_CHANGE:
            rssi_sum = 0;
            averages = 0;
            sp->channel++;
            if (sp->channel == (sp->chan_max - sp->chan_min + 1))
                sp->channel = 0;
            sp->rssi[sp->channel] = 0;
            _scan_next();
            sp->scanState = SCAN_GET_RSSI;
            return CHANNEL_LOCK_TIME;
        case SCAN_GET_RSSI:
            rssi_value = _scan_rssi();
            if (sp->averaging > 0) {
                rssi_sum += rssi_value;
                if (averages >= INTERNAL_AVERAGE * sp->averaging)
                    rssi_sum -= sp->rssi[sp->channel];
                else
                    averages++;
                sp->rssi[sp->channel] = (rssi_sum + averages / 2) / averages;  // exponential smoothing
            } else {
                if (rssi_value > sp->rssi[sp->channel])
                    sp->rssi[sp->channel] = rssi_value;
            }
            if (averages < INTERNAL_AVERAGE * sp->averaging)
                return AVERAGE_INTVL;
            sp->scanState = SCAN_CHANNEL_CHANGE;
    }
    return 50;
}

static void scan_start()
{
    CLOCK_StopTimer();
    CLOCK_StartTimer(1250, scan_cb);
}

static void initialize()
{
    sp->chan_min = MIN_RADIOCHANNEL;
    sp->chan_max = MAX_RADIOCHANNEL;
    rssi_sum = 0;
    averages = 0;
    CYRF_Reset();
    cyrf_init();
    CYRF_SetTxRxMode(RX_EN);  // Receive mode
    scan_start();
}

uintptr_t SCANNER_CYRF_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (CYRF_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 0;
        case PROTOCMD_BIND: return 0;
        case PROTOCMD_NUMCHAN: return 1;
        case PROTOCMD_DEFAULT_NUMCHAN: return 1;
        case PROTOCMD_CURRENT_ID:  return 0;
        case PROTOCMD_GETOPTIONS:
            return 0;
        case PROTOCMD_SETOPTIONS:
            break;
        case PROTOCMD_TELEMETRYSTATE:
            return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP:
            return EATRG;
        default: break;
    }
    return 0;
}

#endif
