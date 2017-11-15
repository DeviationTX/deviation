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
  #define FRSKY1WAY_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "telemetry.h"

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CC2500

#include "iface_cc2500.h"

#if	0
static const char * const frsky_opts[] = {
  _tr_noop("Freq-Fine"),  "-127", "127", NULL,
  _tr_noop("Freq-Course"),  "-127", "127", NULL,
  NULL
};
#endif
enum {
    PROTO_OPTS_FREQFINE = 0,
    PROTO_OPTS_FREQCOURSE = 1,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

static u8 packet[16];
static u32 state;
static u32 seed;
static u32 fixed_id;
static u8 crc;
static s8 course;
static s8 fine;

enum {
    FRSKY_BIND        = 0,
    FRSKY_BIND_DONE  = 200,
    FRSKY_DATA1,
    FRSKY_DATA2,
    FRSKY_DATA3,
    FRSKY_DATA4,
    FRSKY_DATA5,
};

static void frsky_init()
{
        CC2500_Reset();

        CC2500_WriteReg(CC2500_17_MCSM1, 0x0c);
        CC2500_WriteReg(CC2500_18_MCSM0, 0x18);
        CC2500_WriteReg(CC2500_06_PKTLEN, 0xff);
        CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);
        CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);
        CC2500_WriteReg(CC2500_3E_PATABLE, 0xfe);
        CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x08);
        CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
        CC2500_WriteReg(CC2500_0D_FREQ2, 0x5c);
        CC2500_WriteReg(CC2500_0E_FREQ1, 0x58);
        CC2500_WriteReg(CC2500_0F_FREQ0, 0x9d + course);
        CC2500_WriteReg(CC2500_10_MDMCFG4, 0xaa);
        CC2500_WriteReg(CC2500_11_MDMCFG3, 0x10);
        CC2500_WriteReg(CC2500_12_MDMCFG2, 0x93);
        CC2500_WriteReg(CC2500_13_MDMCFG1, 0x23);
        CC2500_WriteReg(CC2500_14_MDMCFG0, 0x7a);
        CC2500_WriteReg(CC2500_15_DEVIATN, 0x41);
        CC2500_WriteReg(CC2500_19_FOCCFG, 0x16);
        CC2500_WriteReg(CC2500_1A_BSCFG, 0x6c);
        CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0x43);
        CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x40);
        CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0x91);
        CC2500_WriteReg(CC2500_21_FREND1, 0x56);
        CC2500_WriteReg(CC2500_22_FREND0, 0x10);
        CC2500_WriteReg(CC2500_23_FSCAL3, 0xa9);
        CC2500_WriteReg(CC2500_24_FSCAL2, 0x0a);
        CC2500_WriteReg(CC2500_25_FSCAL1, 0x00);
        CC2500_WriteReg(CC2500_26_FSCAL0, 0x11);
        CC2500_WriteReg(CC2500_29_FSTEST, 0x59);
        CC2500_WriteReg(CC2500_2C_TEST2, 0x88);
        CC2500_WriteReg(CC2500_2D_TEST1, 0x31);
        CC2500_WriteReg(CC2500_2E_TEST0, 0x0b);
        CC2500_WriteReg(CC2500_03_FIFOTHR, 0x07);
        CC2500_WriteReg(CC2500_09_ADDR, 0x00);

        CC2500_SetTxRxMode(TX_EN);
        CC2500_SetPower(Model.tx_power);

        CC2500_Strobe(CC2500_SIDLE);    // Go to idle...
        //CC2500_WriteReg(CC2500_02_IOCFG0,   0x06);
        //CC2500_WriteReg(CC2500_0A_CHANNR, 0x06);
#if 0
        CC2500_WriteReg(CC2500_02_IOCFG0,   0x01); // reg 0x02: RX complete interrupt
        CC2500_WriteReg(CC2500_17_MCSM1,    0x0C); // reg 0x17: Stay in rx after packet complete
        CC2500_WriteReg(CC2500_18_MCSM0,    0x18); // reg 0x18: Calibrate when going from idle to rx or tx, po timeout count = 64
        CC2500_WriteReg(CC2500_06_PKTLEN,   62);   // Leave room for appended status bytes
        CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05); // reg 0x08: CRC_EN = 1, Length_config = 1 (variable length)
        CC2500_WriteReg(CC2500_3E_PATABLE,  0xFF);
        CC2500_WriteReg(CC2500_0B_FSCTRL1,  0x08); // reg 0x0B: 203 KHz IF
        CC2500_WriteReg(CC2500_0C_FSCTRL0,  0x00); // reg 0x0C

//      CC2500_WriteReg(CC2500_0D_FREQ2,    0x5C); // reg 0x0D
//      CC2500_WriteReg(CC2500_0E_FREQ1,    0x76); // reg 0x0E
//      CC2500_WriteReg(CC2500_0F_FREQ0,    0x27); // reg 0x0F
        CC2500_WriteReg(CC2500_0D_FREQ2,    0x5C); // reg 0x0D     hack: Due to a bit high xtal we shift this down by around 70 khz
        CC2500_WriteReg(CC2500_0E_FREQ1,    0x75); // reg 0x0E
        CC2500_WriteReg(CC2500_0F_FREQ0,    0x6A); // reg 0x0F
        CC2500_WriteReg(CC2500_10_MDMCFG4,  0xAA); // reg 0x10
        CC2500_WriteReg(CC2500_11_MDMCFG3,  0x39); // reg 0x11
        CC2500_WriteReg(CC2500_12_MDMCFG2,  0x11); // reg 0x12
        CC2500_WriteReg(CC2500_13_MDMCFG1,  0x23); // reg 0x13
        CC2500_WriteReg(CC2500_14_MDMCFG0,  0x7A); // reg 0x14
        CC2500_WriteReg(CC2500_15_DEVIATN,  0x42); // reg 0x15
        CC2500_WriteReg(CC2500_19_FOCCFG,   0x16); // reg 0x19
        CC2500_WriteReg(CC2500_1A_BSCFG,    0x6C); // reg 0x1A
        CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0x03); // reg 0x1B
        CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x40); // reg 0x1C
        CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0x91); // reg 0x1D
        CC2500_WriteReg(CC2500_21_FREND1,   0x56); // reg 0x21: Default POR value
        CC2500_WriteReg(CC2500_22_FREND0,   0x10); // reg 0x22: Default POR value
        CC2500_WriteReg(CC2500_23_FSCAL3,   0xA9); // reg 0x23: Default POR value
        CC2500_WriteReg(CC2500_24_FSCAL2,   0x05); // reg 0x24: Default POR value
        CC2500_WriteReg(CC2500_25_FSCAL1,   0x00); // reg 0x25
        CC2500_WriteReg(CC2500_26_FSCAL0,   0x11); // reg 0x26
        CC2500_WriteReg(CC2500_29_FSTEST,   0x59); // reg 0x29
        CC2500_WriteReg(CC2500_2C_TEST2,    0x88); // reg 0x2C
        CC2500_WriteReg(CC2500_2D_TEST1,    0x31); // reg 0x2D
        CC2500_WriteReg(CC2500_2E_TEST0,    0x0B); // reg 0x2E
        CC2500_WriteReg(CC2500_03_FIFOTHR,  0x0F); // reg 0x03: Use max rx fifo
        CC2500_WriteReg(CC2500_09_ADDR,     0x03); // reg 0x09: FrSky bind address is 0x0301 on channel 0
        CC2500_Strobe(CC2500_SIDLE);    // Go to idle...



        CC2500_WriteReg(CC2500_07_PKTCTRL1,0x0D);  // reg 0x07 hack: Append status, filter by address, auto-flush on bad crc, PQT=0
        CC2500_WriteReg(CC2500_0C_FSCTRL0, 0);     // Frequency offset...
        CC2500_WriteReg(CC2500_0A_CHANNR, 0);
#endif

}

static u8 crc8(u32 result, u8 *data, int len)
{
    int polynomial = 0x07;
    for(int i = 0; i < len; i++) {
        result = result ^ data[i];
        for(int j = 0; j < 8; j++) {
            if(result & 0x80) {
                result = (result << 1) ^ polynomial;
            } else {
                result = result << 1;
            }
        }
    }
    return result & 0xff;
}

static u8 crc8_le(u32 _result, u8 *data, int len)
{
    u32 polynomial = 0x83; //x^9 + x^8 + x^7 + 1 
    u32 result = 0;
    for(int i = 0; i < 8; i++) {
        result = (result << 1) | (_result & 0x01);
        _result >>= 1;
    }
    for(int i = 0; i < len; i++) {
        result = result ^ data[i];
        for(int j = 0; j < 8; j++) {
            if(result & 0x01) {
                result = (result >> 1) ^ polynomial;
            } else {
                result = result >> 1;
            }
        }
    }
    return result & 0xff;
}

static void build_bind_packet_1way()
{
    //0e 03 01 57 12 00 06 0b 10 15 1a 00 00 00 61
    packet[0] = 0x0e;                //Length
    packet[1] = 0x03;                //Packet type
    packet[2] = 0x01;                //Packet type
    packet[3] = fixed_id & 0xff;
    packet[4] = fixed_id >> 8;
    packet[5] = ((state - FRSKY_BIND) % 10) * 5;
    packet[6] = packet[5] * 5 + 6;
    packet[7] = packet[5] * 5 + 11;
    packet[8] = packet[5] * 5 + 16;
    packet[9] = packet[5] * 5 + 21;
    packet[10] = packet[5] * 5 + 26;
    packet[11] = 0x00;
    packet[12] = 0x00;
    packet[13] = 0x00;
    packet[14] = crc8(0x93, packet, 14);
}

static u8 calc_channel()
{
    seed = (seed * 0xaa) % 0x7673;
    return (seed & 0xff) % 0x32;
}

static void build_data_packet_1way()
{
    int idx = 0; // transmit lower channels
    packet[0] = 0x0e;
    packet[1] = fixed_id & 0xff;
    packet[2] = fixed_id >> 8;
    packet[3] = seed & 0xff;
    packet[4] = seed >> 8;
    if (state == FRSKY_DATA1 || state == FRSKY_DATA3)
        packet[5] = 0x0f;
    else if(state == FRSKY_DATA2 || state == FRSKY_DATA4)
    {
        packet[5] = 0xf0;
        idx=4; // transmit upper channels
    }
    else
        packet[5] = 0x00;
    for(int i = 0; i < 4; i++) {
        if(idx + i >= Model.num_channels) {
            packet[2*i + 6] = 0xca;
            packet[2*i + 7] = 0x08;
        } else {
            s32 value = (s32)Channels[i + idx] * 600 / CHAN_MAX_VALUE + 0x8ca; //1500 * 1.5 = 2250 = 0x8ca
            packet[2*i + 6] = value & 0xff;
            packet[2*i + 7] = value >> 8;
        }
    }
    packet[14] = crc8(crc, packet, 14);
//for(int i = 0; i < 15; i++) printf("%02x ", packet[i]); printf("\n");
}
static u16 frsky_cb()
{
    if (state < FRSKY_BIND_DONE) {
        build_bind_packet_1way();
        CC2500_Strobe(CC2500_SIDLE);
        CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
        CC2500_WriteData(packet, packet[0]+1);
        state++;
        return 53460;
    }
    if (state == FRSKY_BIND_DONE) {
        state++;
        PROTOCOL_SetBindState(0);
    }
    if (state >= FRSKY_DATA1) {
        u8 chan = calc_channel();
        CC2500_Strobe(CC2500_SIDLE);
        if (fine != (s8)Model.proto_opts[PROTO_OPTS_FREQFINE] || course != (s8)Model.proto_opts[PROTO_OPTS_FREQCOURSE]) {
            course = Model.proto_opts[PROTO_OPTS_FREQCOURSE];
            fine   = Model.proto_opts[PROTO_OPTS_FREQFINE];
            CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
            CC2500_WriteReg(CC2500_0F_FREQ0, 0x9d + course);
        }
        CC2500_WriteReg(CC2500_0A_CHANNR, chan * 5 + 6);
        build_data_packet_1way();

        if (state == FRSKY_DATA5) {
            CC2500_SetPower(Model.tx_power);
            state = FRSKY_DATA1;
        } else {
            state++;
        }

        CC2500_WriteData(packet, packet[0]+1);
        return 9006;
    }
        
    return 0;
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static void get_tx_id()
{
    u32 lfsr = 0x7649eca9ul;

    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
    for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
        rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    fixed_id = rand32_r(&lfsr, 0) & 0x7fff;
    //fixed_id = 0x1257;
    u8 data[2] = {(fixed_id >> 8) & 0xff, fixed_id & 0xff};
    crc = crc8_le(0x6b, data, 2);
    //crc = 0xa6;
}

static void initialize(int bind)
{
    CLOCK_StopTimer();
    course = (int)Model.proto_opts[PROTO_OPTS_FREQCOURSE];
    fine = Model.proto_opts[PROTO_OPTS_FREQFINE];
    get_tx_id();
    printf("%04x - %02x\n", fixed_id, crc);
    frsky_init();
    seed = 1;
    if (bind) {
        PROTOCOL_SetBindState(0xFFFFFFFF);
        state = FRSKY_BIND;
    } else {
        state = FRSKY_DATA1;
    }
    CLOCK_StartTimer(10000, frsky_cb);
}

const void *FRSKY1WAY_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(CC2500_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return 0; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)8L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return frsky_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
