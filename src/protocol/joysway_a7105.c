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
  //Allows the linker to properly relocate
  #define FLYSKY_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"
#include "telemetry.h"

#ifdef MODULAR
  #pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_A7105


#define EVEN_ODD 0x00
//#define EVEN_ODD 0x01
static const u8 A7105_regs[] = {
    0x00, 0x62,   -1, 0x0f, 0x00,  -1 ,  -1 , 0x00,     0x00, 0x05, 0x00, 0x01, 0x00, 0xf5, 0x00, 0x15,
    0x9e, 0x4b, 0x00, 0x03, 0x56, 0x2b, 0x12, 0x4a,     0x02, 0x80, 0x80, 0x00, 0x0e, 0x91, 0x03, 0x0f,
    0x16, 0x2a, 0x00,  -1,    -1,   -1, 0x3a, 0x06,     0x1f, 0x47, 0x80, 0x01, 0x05, 0x45, 0x18, 0x00,
    0x01, 0x0f, 0x00
};

static u32 id;
static u8 packet[16];
static u8 counter;
static u8 next_ch;

static int joysway_init()
{
    int i;
    u8 if_calibration1;
    //u8 vco_calibration0;
    //u8 vco_calibration1;

    counter = 0;
    next_ch = 0x30;
    id = 0xf82dcaa0;

    for (i = 0; i < 0x33; i++)
        if((s8)A7105_regs[i] != -1)
            A7105_WriteReg(i, A7105_regs[i]);
    A7105_WriteID(0x5475c52a);

    A7105_Strobe(A7105_PLL);

    //IF Filter Bank Calibration
    A7105_WriteReg(0x02, 1);
    A7105_ReadReg(0x02);
    u32 ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    A7105_Strobe(A7105_STANDBY);
    if_calibration1 = A7105_ReadReg(0x22);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return 0;
    }

    //VCO Current Calibration
    A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet
    A7105_WriteReg(0x25, 0x09); //Recomended calibration from A7105 Datasheet

    A7105_WriteID(id);
    A7105_Strobe(A7105_PLL);
    A7105_WriteReg(0x02, 1);
    ms = CLOCK_getms();
    CLOCK_ResetWatchdog();
    while(CLOCK_getms()  - ms < 500) {
        if(! A7105_ReadReg(0x02))
            break;
    }
    if (CLOCK_getms() - ms >= 500)
        return 0;
    A7105_Strobe(A7105_STANDBY);
    if_calibration1 = A7105_ReadReg(0x22);
    if(if_calibration1 & A7105_MASK_FBCF) {
        //Calibration failed...what do we do?
        return 0;
    }
    A7105_WriteReg(0x24, 0x13); //Recomended calibration from A7105 Datasheet
    A7105_WriteReg(0x25, 0x09); //Recomended calibration from A7105 Datasheet

    A7105_SetTxRxMode(TX_EN);
    A7105_SetPower(Model.tx_power);

    A7105_Strobe(A7105_STANDBY);
    return 1;
}

static void flysky_build_packet()
{
    int i;
    //-100% =~ 0x03e8
    //+100% =~ 0x07ca
    //Calculate:
    //Center = 0x5d9
    //1 %    = 5
    packet[0] = counter == 0 ? 0xdd : 0xff;
    packet[1] = (id >> 24) & 0xff;
    packet[2] = (id >> 16) & 0xff;
    packet[3] = (id >>  8) & 0xff;
    packet[4] = (id >>  0) & 0xff;
    packet[5] = 0x00;
    static const int chmap[4] = {6, 7, 10, 11};
    for (i = 0; i < 4; i++) {
        if (i >= Model.num_channels) {
            packet[chmap[i]] = 0x64;
            continue;
        }
        s32 value = (s32)Channels[i] * 0x66 / CHAN_MAX_VALUE + 0x66;
        if (value < 0)
            value = 0;
        if (value > 0xff)
            value = 0xff;
        packet[chmap[i]] = value;
    }
    packet[8] = 0x64;
    packet[9] = 0x64;
    packet[12] = 0x64;
    packet[13] = 0x64;
    packet[14] = counter == 0 ? 0x30 : 0xaa;
    u8 value = 0;
    for (int i = 0; i < 15; i++) {
        value += packet[i];
    }
    packet[15] = value;
}

MODULE_CALLTYPE
static u16 joysway_cb()
{
    u8 ch;
    if (counter == 254) {
        counter = 0;
        A7105_WriteID(0x5475c52a);
        ch = 0x0a;
    } else if (counter == 2) {
        A7105_WriteID(id);
        ch = 0x30;
    } else {
        if ((counter & 0x01) ^ EVEN_ODD) {
            ch = 0x30;
        } else {
            ch = next_ch;
        }
    }
    if (! ((counter & 0x01) ^ EVEN_ODD)) {
        next_ch++;
        if (next_ch == 0x45)
            next_ch = 0x30;
    }
    flysky_build_packet();
    A7105_Strobe(A7105_STANDBY);
    A7105_WriteData(packet, 16, ch);
    counter++;
    return 6000;
}

static void initialize() {
    CLOCK_StopTimer();
    while(1) {
        A7105_Reset();
        CLOCK_ResetWatchdog();
        if (joysway_init())
            break;
    }
    //if (Model.fixed_id) {
    //    id = Model.fixed_id;
    //} else {
    //    id = (Crc(&Model, sizeof(Model)) + Crc(&Transmitter, sizeof(Transmitter))) % 999999;
    //}
    CLOCK_StartTimer(2400, joysway_cb);
}

const void *JOYSWAY_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(A7105_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)4L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)4L;
        case PROTOCMD_CURRENT_ID: return (void *)((unsigned long)id);
        case PROTOCMD_GETOPTIONS:
            return NULL;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
