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
  #define SKYARTEC_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CC2500

#include "iface_cc2500.h"

static u8 packet[20];
static u32 state;
static u32 fixed_id;
static u32 bind_count;
#define TX_ADDR ((fixed_id >> 16) & 0xff)
#define TX_CHANNEL ((fixed_id >> 24) & 0xff)

enum {
    SKYARTEC_PKT1 = 0,
    SKYARTEC_SLEEP1, 
    SKYARTEC_PKT2,
    SKYARTEC_SLEEP2, 
    SKYARTEC_PKT3,
    SKYARTEC_SLEEP3, 
    SKYARTEC_PKT4,
    SKYARTEC_SLEEP4, 
    SKYARTEC_PKT5,
    SKYARTEC_SLEEP5, 
    SKYARTEC_PKT6,
    SKYARTEC_LAST,
};

static void skyartec_init()
{
        CC2500_Reset();

        CC2500_WriteReg(CC2500_16_MCSM2, 0x07);
        CC2500_WriteReg(CC2500_17_MCSM1, 0x30);
        CC2500_WriteReg(CC2500_1E_WOREVT1, 0x87);
        CC2500_WriteReg(CC2500_1F_WOREVT0, 0x6b);
        CC2500_WriteReg(CC2500_20_WORCTRL, 0xf8);
        CC2500_WriteReg(CC2500_2A_PTEST, 0x7f);
        CC2500_WriteReg(CC2500_2B_AGCTEST, 0x3f);
        CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x09);
        CC2500_WriteReg(CC2500_0C_FSCTRL0, 0x00);
        CC2500_WriteReg(CC2500_0D_FREQ2, 0x5d);
        CC2500_WriteReg(CC2500_0E_FREQ1, 0x93);
        CC2500_WriteReg(CC2500_0F_FREQ0, 0xb1);
        CC2500_WriteReg(CC2500_10_MDMCFG4, 0x2d);
        CC2500_WriteReg(CC2500_11_MDMCFG3, 0x20);
        CC2500_WriteReg(CC2500_12_MDMCFG2, 0x73);
        CC2500_WriteReg(CC2500_13_MDMCFG1, 0x22);
        CC2500_WriteReg(CC2500_14_MDMCFG0, 0xf8);
        CC2500_WriteReg(CC2500_0A_CHANNR, 0xcd);
        CC2500_WriteReg(CC2500_15_DEVIATN, 0x50);
        CC2500_WriteReg(CC2500_21_FREND1, 0xb6);
        CC2500_WriteReg(CC2500_22_FREND0, 0x10);
        CC2500_WriteReg(CC2500_18_MCSM0, 0x18);
        CC2500_WriteReg(CC2500_19_FOCCFG, 0x1d);
        CC2500_WriteReg(CC2500_1A_BSCFG, 0x1c);
        CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xc7);
        CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x00);
        CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xb2);
        CC2500_WriteReg(CC2500_23_FSCAL3, 0xea);
        CC2500_WriteReg(CC2500_24_FSCAL2, 0x0a);
        CC2500_WriteReg(CC2500_25_FSCAL1, 0x00);
        CC2500_WriteReg(CC2500_26_FSCAL0, 0x11);
        CC2500_WriteReg(CC2500_29_FSTEST, 0x59);
        CC2500_WriteReg(CC2500_2C_TEST2, 0x88);
        CC2500_WriteReg(CC2500_2D_TEST1, 0x31);
        CC2500_WriteReg(CC2500_2E_TEST0, 0x0b);
        CC2500_WriteReg(CC2500_00_IOCFG2, 0x0b);
        CC2500_WriteReg(CC2500_02_IOCFG0, 0x06);
        CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x05);
        CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);
        CC2500_WriteReg(CC2500_09_ADDR, 0x43);
        CC2500_WriteReg(CC2500_06_PKTLEN, 0xff);
        CC2500_WriteReg(CC2500_04_SYNC1, 0x13);
        CC2500_WriteReg(CC2500_05_SYNC0, 0x18);
        CC2500_Strobe(CC2500_SFTX);
        CC2500_Strobe(CC2500_SFRX);
        CC2500_Strobe(CC2500_SXOFF);
        CC2500_Strobe(CC2500_SIDLE);
}

static void add_pkt_suffix()
{
    int xor1 = 0;
    int xor2 = 0;
    for(int i = 3; i <= 16; i++)
        xor1 ^= packet[i];
    for(int i = 3; i <= 14; i++)
        xor2 ^= packet[i];

    int sum = packet[3] + packet[5] + packet[7] + packet[9] + packet[11] + packet[13];
    packet[17] = xor1;
    packet[18] = xor2;
    packet[19] = sum & 0xff;
}

static void send_data_packet()
{
    //13 c5 01 0259 0168 0000 0259 030c 021a 0489 f3 7e 0a
    packet[0] = 0x13;                //Length
    packet[1] = TX_ADDR;             //Tx Addr?
    packet[2] = 0x01;                //???
    for(int i = 0; i < 7; i++) {
        s32 value = (s32)Channels[i] * 0x280 / CHAN_MAX_VALUE + 0x280;
        if(value < 0)
            value = 0;
        if(value > 0x280)
            value = 0x280;
        packet[3+2*i] = value >> 8;
        packet[4+2*i] = value & 0xff;
    }
    add_pkt_suffix();
    CC2500_WriteReg(CC2500_04_SYNC1, 0x2f);
    CC2500_WriteReg(CC2500_05_SYNC0, 0x4a);
    CC2500_WriteReg(CC2500_09_ADDR, TX_ADDR);
    CC2500_WriteReg(CC2500_0A_CHANNR, TX_CHANNEL);
    CC2500_WriteData(packet, 20);
}

static void send_bind_packet()
{
    //0b 7d 01 01 b2 c5 4a 2f 00 00 c5 d6
    packet[0] = 0x0b;       //Length
    packet[1] = 0x7d;
    packet[2] = 0x01;
    packet[3] = 0x01;
    packet[4] = (fixed_id >> 24) & 0xff;
    packet[5] = (fixed_id >> 16) & 0xff;
    packet[6] = (fixed_id >> 8)  & 0xff;
    packet[7] = (fixed_id >> 0)  & 0xff;
    packet[8] = 0x00;
    packet[9] = 0x00;
    packet[10] = 0xc5;
    packet[11] = 0xd6;
    CC2500_WriteReg(CC2500_04_SYNC1, 0x7d);
    CC2500_WriteReg(CC2500_05_SYNC0, 0x7d);
    CC2500_WriteReg(CC2500_09_ADDR, 0x7d);
    CC2500_WriteReg(CC2500_0A_CHANNR, 0x7d);
    CC2500_WriteData(packet, 12);
}

static u16 skyartec_cb()
{
    if (state & 0x01) {
        CC2500_Strobe(CC2500_SIDLE);
        state = (state == SKYARTEC_LAST) ? SKYARTEC_PKT1 : state + 1;
        return 3000;
    }
    if (state == SKYARTEC_PKT1 && bind_count) {
        send_bind_packet();
    } else {
        send_data_packet();
    }
    return 3000;
}

static void initialize()
{
    CLOCK_StopTimer();
    skyartec_init();
    fixed_id = 0xb2c54a2f;
    bind_count = 10000;
    state = SKYARTEC_PKT1;

    CLOCK_StartTimer(10000, skyartec_cb);
}

const void *SKYARTEC_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT: return 0;
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; //Always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return (void *)7L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)7L;
        case PROTOCMD_CURRENT_ID: return 0;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)-1;
        default: break;
    }
    return 0;
}
#endif
