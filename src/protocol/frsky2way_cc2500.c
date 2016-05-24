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
  #define FRSKY2WAY_Cmds PROTO_Cmds
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

static const char * const frsky_opts[] = {
  _tr_noop("Telemetry"),  _tr_noop("On"), _tr_noop("Off"), NULL,
  _tr_noop("Freq-Fine"),  "-127", "127", NULL,
  _tr_noop("Freq-Course"),  "-127", "127", NULL,
  _tr_noop("AD2GAIN"),  "1", "255", NULL,
  NULL
};
enum {
    PROTO_OPTS_TELEM = 0,
    PROTO_OPTS_FREQFINE = 1,
    PROTO_OPTS_FREQCOURSE = 2,
    PROTO_OPTS_AD2GAIN = 3,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#define TELEM_ON 0
#define TELEM_OFF 1
static u8 packet[40];
static u32 state;
static u8 counter;
static u32 fixed_id;
static s8 course;
static s8 fine;


enum {
    FRSKY_BIND        = 0,
#ifdef EMULATOR
    FRSKY_BIND_DONE  = 10,
#else
    FRSKY_BIND_DONE  = 1000,
#endif
    FRSKY_DATA1,
    FRSKY_DATA2,
    FRSKY_DATA3,
    FRSKY_DATA4,
    FRSKY_DATA5,
};

static void frsky2way_init(int bind)
{
        CC2500_Reset();
        CC2500_WriteReg(CC2500_17_MCSM1, 0x0c);
        CC2500_WriteReg(CC2500_18_MCSM0, 0x18);
        CC2500_WriteReg(CC2500_06_PKTLEN, 0x19);
        CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04);
        CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x05);
        CC2500_WriteReg(CC2500_3E_PATABLE, 0xff);
        CC2500_WriteReg(CC2500_0B_FSCTRL1, 0x08);
        CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
        CC2500_WriteReg(CC2500_0D_FREQ2, 0x5c);
        CC2500_WriteReg(CC2500_0E_FREQ1, 0x76);
        CC2500_WriteReg(CC2500_0F_FREQ0, 0x27 + course);
        CC2500_WriteReg(CC2500_10_MDMCFG4, 0xaa);
        CC2500_WriteReg(CC2500_11_MDMCFG3, 0x39);
        CC2500_WriteReg(CC2500_12_MDMCFG2, 0x11);
        CC2500_WriteReg(CC2500_13_MDMCFG1, 0x23);
        CC2500_WriteReg(CC2500_14_MDMCFG0, 0x7a);
        CC2500_WriteReg(CC2500_15_DEVIATN, 0x42);
        CC2500_WriteReg(CC2500_19_FOCCFG, 0x16);
        CC2500_WriteReg(CC2500_1A_BSCFG, 0x6c);
        CC2500_WriteReg(CC2500_1B_AGCCTRL2, bind ? 0x43 : 0x03);
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

        CC2500_WriteReg(CC2500_09_ADDR, bind ? 0x03 : (fixed_id & 0xff));
        CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x04); //Should be 0x05 but the filter isn't working

        CC2500_Strobe(CC2500_SIDLE);    // Go to idle...

        CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
        CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
        //CC2500_WriteReg(CC2500_3E_PATABLE, 0x50);
        CC2500_Strobe(CC2500_SFRX);
}

static int get_chan_num(int idx)
{
    int ret = (idx * 0x1e) % 0xeb;
    if(idx == 3 || idx == 23 || idx == 47)
        ret++;
    if(idx > 47)
        return 0;
    return ret;
}

static void frsky2way_build_bind_packet()
{
    //11 03 01 d7 2d 00 00 1e 3c 5b 78 00 00 00 00 00 00 01
    //11 03 01 19 3e 00 02 8e 2f bb 5c 00 00 00 00 00 00 01
    packet[0] = 0x11;                //Length
    packet[1] = 0x03;                //Packet type
    packet[2] = 0x01;                //Packet type
    packet[3] = fixed_id & 0xff;
    packet[4] = fixed_id >> 8;
    int idx = ((state - FRSKY_BIND) % 10) * 5;
    packet[5] = idx;
    packet[6] = get_chan_num(idx++);
    packet[7] = get_chan_num(idx++);
    packet[8] = get_chan_num(idx++);
    packet[9] = get_chan_num(idx++);
    packet[10] = get_chan_num(idx++);
    packet[11] = 0x00;
    packet[12] = 0x00;
    packet[13] = 0x00;
    packet[14] = 0x00;
    packet[15] = 0x00;
    packet[16] = 0x00;
    packet[17] = 0x01;
}

static void frsky2way_build_data_packet()
{
    //11 d7 2d 22 00 01 c9 c9 ca ca 88 88 ca ca c9 ca 88 88
    //11 57 12 00 00 01 f2 f2 f2 f2 06 06 ca ca ca ca 18 18
    packet[0] = 0x11;             //Length
    packet[1] = fixed_id & 0xff;
    packet[2] = fixed_id >> 8;
    packet[3] = counter;
    packet[4] = 0x00;
    packet[5] = 0x01;

    packet[10] = 0;
    packet[11] = 0;
    packet[16] = 0;
    packet[17] = 0;
    for(int i = 0; i < 8; i++) {
        s32 value;
        if(i >= Model.num_channels) {
            value = 0x8ca;
        } else {
            value = (s32)Channels[i] * 600 / CHAN_MAX_VALUE + 0x8ca; // 0-2047, 0 = 817, 1024 = 1500, 2047 = 2182
        }                                                            // H (0)7-4, L (0)3-0, H (1)3-0, L (0)11-8, H (1)11-8, L (1)7-4 etc
        if(i < 4) {
            packet[6+i] = value & 0xff;
            packet[10+(i>>1)] |= ((value >> 8) & 0x0f) << (4 *(i & 0x01));
        } else {
            packet[8+i] = value & 0xff;
            packet[16+((i-4)>>1)] |= ((value >> 8) & 0x0f) << (4 * ((i-4) & 0x01));
        }
    }
    //if(counter == 0) {
    //    for(int i = 0; i < 18; i++)
    //        printf("%02x ", packet[i]);
    //    printf("\n");
    //}
}

#if HAS_EXTENDED_TELEMETRY

#include "frsky_d_telem._c"


static TS_STATE ts_state;    // file scope so can reset on sequence error
static void frsky_parse_telem_stream(u8 byte) {
    static u8 data_id;
    static u8 lowByte, highByte;


    if (ts_state == TS_DATA_END) {
        if (byte == 0x5e) {
            processHubPacket(data_id, (highByte << 8) + lowByte);
            ts_state = TS_DATA_ID;
        } else {
            ts_state = TS_IDLE;
        }
        return;
    }
    if (byte == 0x5e) {
        ts_state = TS_DATA_ID;
        return;
    }
    if (ts_state == TS_IDLE) {
        return;
    }
    if (ts_state & TS_XOR) {
        byte = byte ^ 0x60;
        ts_state = (TS_STATE)(ts_state - TS_XOR);
    }
    else if (byte == 0x5d) {
        ts_state = (TS_STATE)(ts_state | TS_XOR);
        return;
    }
    if (ts_state == TS_DATA_ID) {
        if (byte > 0x3f) {
            ts_state = TS_IDLE;
        }
        else {
            data_id = byte;
            ts_state = TS_DATA_LOW;
        }
        return;
    }
    if (ts_state == TS_DATA_LOW) {
        lowByte = byte;
        ts_state = TS_DATA_HIGH;
        return;
    }
    if (ts_state == TS_DATA_HIGH) {
        highByte = byte;
        ts_state = TS_DATA_END;
    }
}

#endif // HAS_EXTENDED_TELEMETRY

static void frsky2way_parse_telem(u8 *pkt, int len)
{
// pkt 0 = length not counting appended status bytes
// pkt 1,2 = fixed_id
// pkt 3 = A1 : 52mV per count; 4.5V = 0x56
// pkt 4 = A2 : 13.4mV per count; 3.0V = 0xE3 on D6FR
// pkt 5 = RSSI
// pkt 6 = number of stream bytes
// pkt 7 = sequence number increments mod 8 with each packet containing stream data
// pkt 8-(8+(pkt[6]-1)) = stream data
// pkt len-2 = downlink RSSI
// pkt len-1 = crc status (bit7 set indicates good), link quality indicator (bits6-0)


// D-series receivers seem to return a lot of bad packets.  Do as much as possible to
// avoid bad data.


    u8 AD2gain = Model.proto_opts[PROTO_OPTS_AD2GAIN];

    // packet checks: sensible length, good CRC, matching fixed id
    if (len != pkt[0] + 3 || pkt[0] < 5
        || !(pkt[len-1] & 0x80)
        || pkt[1] != (fixed_id & 0xff) || pkt[2] != ((fixed_id >> 8) & 0xff)) {
            return;
    }

    //Get voltage A1 (52mv/count)
    Telemetry.value[TELEM_FRSKY_VOLT1] = pkt[3] * 52 / 10; //In 1/100 of Volts
    TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
    //Get voltage A2 (~13.2mv/count) (Docs say 1/4 of A1)
    Telemetry.value[TELEM_FRSKY_VOLT2] = pkt[4] * (132*AD2gain) / 1000; //In 1/100 of Volts *(A2gain/10)
    TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT2);

    Telemetry.value[TELEM_FRSKY_RSSI] = pkt[5]; 	// Value in Db
    TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);

#if HAS_EXTENDED_TELEMETRY
    static u8 sequence;

    if (pkt[0] < 7) return;   // be paranoid about packet length

    if (pkt[6] && pkt[6] <= pkt[0]-7) {   // be paranoid about packet length
        if (pkt[7] != sequence) {
            ts_state = TS_IDLE;
            sequence = pkt[7];    // should be able to recover in middle of sequence
        }
        sequence = (sequence + 1) % 8;
            
        for(int i=8; i < 8+pkt[6]; i++)
            frsky_parse_telem_stream(pkt[i]);
    }
#endif // HAS_EXTENDED_TELEMETRY
}


static u16 frsky2way_cb()
{
    if (state < FRSKY_BIND_DONE) {
        frsky2way_build_bind_packet();
        CC2500_Strobe(CC2500_SIDLE);
        CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
        CC2500_WriteData(packet, packet[0]+1);
        state++;
#ifdef EMULATOR
        return 90;
#else
        return 9000;
#endif
    }
    if (state == FRSKY_BIND_DONE) {
        state = FRSKY_DATA2;
        PROTOCOL_SetBindState(0);
        frsky2way_init(0);
        counter = 0;
    } else if (state == FRSKY_DATA5) {
        CC2500_Strobe(CC2500_SRX);
        state = FRSKY_DATA1;
#ifdef EMULATOR
        return 92;
#else
        return 9200;
#endif
    }
    counter = (counter + 1) % 188;
    if (state == FRSKY_DATA4) {
        //telemetry receive
        CC2500_SetTxRxMode(RX_EN);
        CC2500_Strobe(CC2500_SIDLE);
        CC2500_WriteReg(CC2500_0A_CHANNR, get_chan_num(counter % 47));
        CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
        state++;
#ifdef EMULATOR
        return 13;
#else
        return 1300;
#endif
    } else {
        if (state == FRSKY_DATA1) {
            unsigned len = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST);
            if (len && len < sizeof(packet)) {
                CC2500_ReadData(packet, len);
                //CC2500_WriteReg(CC2500_0C_FSCTRL0, CC2500_ReadReg(CC2500_32_FREQEST));
                //parse telemetry packet here
                frsky2way_parse_telem(packet, len);
            }
#ifdef EMULATOR
static u8 testdata[] = {
 0x11, 0x5a, 0xdf, 0x45, 0x00, 0x4b, 0x0a, 0x00, 0x10, 0x46, 0x00, 0x5e, 0x04, 0x64, 0x00, 0x5e, 0x02, 0x1a, 0x40, 0xad,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4a, 0x0a, 0x01, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x4c, 0xad,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4b, 0x0a, 0x02, 0x05, 0xec, 0xff, 0x5e, 0x10, 0x46, 0x00, 0x5e, 0x02, 0x1a, 0x3e, 0xae,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4b, 0x0a, 0x03, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x4d, 0xae,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4c, 0x0a, 0x04, 0x05, 0xec, 0xff, 0x5e, 0x04, 0x64, 0x00, 0x5e, 0x10, 0x46, 0x3e, 0xae,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4b, 0x0a, 0x05, 0x00, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x4d, 0xac,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4a, 0x0a, 0x06, 0x10, 0x46, 0x00, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x05, 0xec, 0x3e, 0xaf,
 0x11, 0x5a, 0xdf, 0x47, 0x00, 0x4b, 0x0a, 0x07, 0xff, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x4d, 0xad,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4c, 0x0a, 0x00, 0x10, 0x46, 0x00, 0x5e, 0x21, 0x64, 0x00, 0x5e, 0x02, 0x1a, 0x4b, 0xad,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4a, 0x0a, 0x01, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x3f, 0xae,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4c, 0x0a, 0x02, 0x05, 0xec, 0xff, 0x5e, 0x10, 0x46, 0x00, 0x5e, 0x21, 0x1a, 0x4b, 0xac,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4b, 0x0a, 0x03, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x3e, 0xac,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4b, 0x0a, 0x04, 0x05, 0xec, 0xff, 0x5e, 0x04, 0x64, 0x00, 0x5e, 0x10, 0x46, 0x4a, 0xae,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4a, 0x0a, 0x05, 0x00, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x41, 0xae,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4b, 0x0a, 0x06, 0x10, 0x46, 0x00, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x05, 0xec, 0x4a, 0xad,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4a, 0x0a, 0x07, 0xff, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x41, 0xac,
 0x11, 0x5a, 0xdf, 0x44, 0x00, 0x4b, 0x00, 0x00, 0xff, 0x5e, 0x02, 0x1a, 0x00, 0x5e, 0x05, 0xec, 0xff, 0x5e, 0x49, 0xae,
 0x00 };
static u8 *data = testdata;

            if (!*data) data = testdata;
            len = *data + 3;
            memcpy(packet, data, len);
            data[1] = fixed_id & 0xff;
            data[2] = fixed_id >> 8;
            data += len;
            frsky2way_parse_telem(packet, len);
#endif //EMULATOR

            CC2500_SetTxRxMode(TX_EN);
            CC2500_SetPower(Model.tx_power);
        }
        CC2500_Strobe(CC2500_SIDLE);
        if (fine != (s8)Model.proto_opts[PROTO_OPTS_FREQFINE] || course != (s8)Model.proto_opts[PROTO_OPTS_FREQCOURSE]) {
            course = Model.proto_opts[PROTO_OPTS_FREQCOURSE];
            fine   = Model.proto_opts[PROTO_OPTS_FREQFINE];
            CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
            CC2500_WriteReg(CC2500_0F_FREQ0, 0x27 + course);
        }
        CC2500_WriteReg(CC2500_0A_CHANNR, get_chan_num(counter % 47));
        CC2500_WriteReg(CC2500_23_FSCAL3, 0x89);
        //CC2500_WriteReg(CC2500_3E_PATABLE, 0xfe);
        CC2500_Strobe(CC2500_SFRX);
        frsky2way_build_data_packet();
        CC2500_WriteData(packet, packet[0]+1);
        state++;
    }
#ifdef EMULATOR
    return state == FRSKY_DATA4 ? 75 : 90;
#else
    return state == FRSKY_DATA4 ? 7500 : 9000;
#endif
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)
static int get_tx_id()
{
    u32 lfsr = 0x7649eca9ul;

    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
    for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
        rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    return rand32_r(&lfsr, 0);
}

static void initialize(int bind)
{
    CLOCK_StopTimer();
    course = (int)Model.proto_opts[PROTO_OPTS_FREQCOURSE];
    fine = Model.proto_opts[PROTO_OPTS_FREQFINE];
    //fixed_id = 0x3e19;
    fixed_id = get_tx_id();
#if HAS_EXTENDED_TELEMETRY
    ts_state = TS_IDLE;
#endif
    frsky2way_init(bind);
    if (bind) {
        PROTOCOL_SetBindState(0xFFFFFFFF);
        state = FRSKY_BIND;
    } else {
        state = FRSKY_BIND_DONE;
    }
#ifdef EMULATOR
    CLOCK_StartTimer(100, frsky2way_cb);
#else
    CLOCK_StartTimer(10000, frsky2way_cb);
#endif
}

const void *FRSKY2WAY_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_CHECK_AUTOBIND: return 0; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)8L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS:
            return frsky_opts;
        case PROTOCMD_TELEMETRYSTATE:
            return (void *)(long)(Model.proto_opts[PROTO_OPTS_TELEM] == TELEM_ON ? PROTO_TELEM_ON : PROTO_TELEM_OFF);
        case PROTOCMD_RESET:
        case PROTOCMD_DEINIT:
            CLOCK_StopTimer();
            return (void *)(CC2500_Reset() ? 1L : -1L);
        default: break;
    }
    return 0;
}
#endif

