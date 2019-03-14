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

#ifdef MODULAR
    // Allows the linker to properly relocate
    #define SCANNER_CYRF_Cmds PROTO_Cmds
    #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "rftools.h"
#include "telemetry.h"

#ifdef PROTO_HAS_CYRF6936
#if SUPPORT_SCANNER

#ifdef MODULAR
    struct Scanner Scanner;
#endif

#define MIN_RADIOCHANNEL    0x00
#define MAX_RADIOCHANNEL    0x62
#define CHANNEL_LOCK_TIME   300  // slow channel requires 270 usec for synthesizer to settle
#define INTERNAL_AVERAGE    3
#define AVERAGE_INTVL       50

static int averages, channel, scan_state;
static u32 rssi_sum;

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
    CYRF_ConfigRFChannel(channel + Scanner.chan_min);
    switch (Scanner.attenuator) {
        case 0: CYRF_WriteRegister(CYRF_06_RX_CFG, 0x4A); break;  // LNA on, ATT off
        case 1: CYRF_WriteRegister(CYRF_06_RX_CFG, 0x0A); break;  // LNA off, ATT off
        default:  CYRF_WriteRegister(CYRF_06_RX_CFG, 0x2A); break;  // LNA off, no ATT on
    }
}

static int _scan_rssi()
{
    if ( !(CYRF_ReadRegister(CYRF_05_RX_CTRL) & 0x80) ) {
        CYRF_WriteRegister(CYRF_05_RX_CTRL, 0x80);  // Prepare to receive
        usleep(1);
        CYRF_ReadRegister(CYRF_13_RSSI);  // dummy read
        usleep(1);
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
    switch (scan_state) {
        case SCAN_CHANNEL_CHANGE:
            rssi_sum = 0;
            averages = 0;
            channel++;
            if (channel == (Scanner.chan_max - Scanner.chan_min + 1))
                channel = 0;
            //if (Scanner.averaging)
            //    Scanner.rssi[channel] = 0;
            _scan_next();
            scan_state = SCAN_GET_RSSI;
            return CHANNEL_LOCK_TIME;
        case SCAN_GET_RSSI:
            rssi_value = _scan_rssi();
            if (Scanner.averaging) {
                rssi_sum += rssi_value;
                if (averages >= INTERNAL_AVERAGE * Scanner.averaging)
                    rssi_sum -= Scanner.rssi[channel];
                else
                    averages++;
                Scanner.rssi[channel] = (rssi_sum + averages / 2) / averages;  // exponential smoothing
            } else {
                if (rssi_value > Scanner.rssi[channel])
                    Scanner.rssi[channel] = rssi_value;
            }
            if (averages < INTERNAL_AVERAGE * Scanner.averaging)
                return AVERAGE_INTVL + rand32() % 50;  // make measurements slightly random in time
            scan_state = SCAN_CHANNEL_CHANGE;
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
    Scanner.chan_min = MIN_RADIOCHANNEL;
    Scanner.chan_max = MAX_RADIOCHANNEL;
    rssi_sum = 0;
    averages = 0;
    channel = 0;
    scan_state = SCAN_CHANNEL_CHANGE;
    memset(Scanner.rssi, 0, sizeof(Scanner.rssi));  // clear old rssi values
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
        case PROTOCMD_NUMCHAN: return 16;
        case PROTOCMD_DEFAULT_NUMCHAN: return 1;
        case PROTOCMD_CURRENT_ID:  return 0;
        case PROTOCMD_GETOPTIONS:
            return 0;
        case PROTOCMD_SETOPTIONS:
            break;
        case PROTOCMD_TELEMETRYSTATE:
            return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP:
            return UNCHG;
        default: break;
    }
    return 0;
}

#endif  // SUPPORT_SCANNER
#endif  // PROTO_CYRF_6936
