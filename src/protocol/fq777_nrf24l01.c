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
  #define FQ777_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"
#include "telemetry.h"

#ifdef MODULAR
  //Some versions of gcc apply this to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 20
#define dbgprintf printf
#else
#define BIND_COUNT 1000
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

#define INITIAL_WAIT    500
#define PACKET_PERIOD   2000
#define PACKET_SIZE     8
#define BIND_COUNT      1000
#define NUM_RF_CHANNELS 4

enum {
    FLAG_RETURN     = 0x40,  // 0x40 when not off, !0x40 when one key return
    FLAG_HEADLESS   = 0x04,
    FLAG_EXPERT     = 0x01,
    FLAG_FLIP       = 0x80,
};

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     //
    CHANNEL6,     // Flip
    CHANNEL7,     //
    CHANNEL8,     //
    CHANNEL9,     // Headless
    CHANNEL10,    // RTH
};

#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10

static const char * const fq777_opts[] = {
    _tr_noop("Format"), "124", NULL,
    NULL
};

enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

enum {
    PROTOOPTS_FORMAT_124,
};

static const u8 ssv_xor[] = {0x80,0x44,0x64,0x75,0x6C,0x71,0x2A,0x36,0x7C,0xF1,0x6E,0x52,0x9,0x9D,0x1F,0x78,0x3F,0xE1,0xEE,0x16,0x6D,0xE8,0x73,0x9,0x15,0xD7,0x92,0xE7,0x3,0xBA};
static const u8 bind_addr [] = {0xe7,0xe7,0xe7,0xe7,0x67};
static const u8 hopping_frequency[NUM_RF_CHANNELS] = {0x4D,0x43,0x27,0x07};
static u16 bind_counter;
static u8 rx_tx_addr[5];
static u32 packet_count;
static u8 hopping_frequency_no;
static u8 tx_power;
static u8 packet[32];

// Bit vector from bit position
#define _BV(bit) (1 << bit)

static const u16 polynomial = 0x1021;
static u16 crc16_update(u16 crc, uint8_t a)
{
	crc ^= a << 8;
    for (uint8_t i = 0; i < 8; ++i)
        if (crc & 0x8000)
            crc = (crc << 1) ^ polynomial;
		else
            crc = crc << 1;
    return crc;
}

static void ssv_pack_dpl(u8 const addr[], u8 pid, u8* len, u8* payload, u8* packed_payload)
{
    u8 i = 0;

    u16 pcf = (*len & 0x3f) << 3;
    pcf |= (pid & 0x3) << 1;
    pcf |= 0x00; // noack field
    
    u8 header[7] = {0};
    header[6] = pcf;
    header[5] = (pcf >> 7) | (addr[0] << 1);
    header[4] = (addr[0] >> 7) | (addr[1] << 1);
    header[3] = (addr[1] >> 7) | (addr[2] << 1);
    header[2] = (addr[2] >> 7) | (addr[3] << 1);
    header[1] = (addr[3] >> 7) | (addr[4] << 1);
    header[0] = (addr[4] >> 7);

    // calculate the crc
    union 
    {
        u8 bytes[2];
        u16 val;
    } crc;

    crc.val=0x3c18;
    for (i = 0; i < 7; ++i)
        crc.val=crc16_update(crc.val,header[i]);
    for (i = 0; i < *len; ++i)
        crc.val=crc16_update(crc.val,payload[i]);

    // encode payload and crc
    // xor with this:
    for (i = 0; i < *len; ++i)
        payload[i] ^= ssv_xor[i];
    crc.bytes[1] ^= ssv_xor[i++];
    crc.bytes[0] ^= ssv_xor[i++];

    // pack the pcf, payload, and crc into packed_payload
    packed_payload[0] = pcf >> 1;
    packed_payload[1] = (pcf << 7) | (payload[0] >> 1);
    
    for (i = 0; i < *len - 1; ++i)
        packed_payload[i+2] = (payload[i] << 7) | (payload[i+1] >> 1);

    packed_payload[i+2] = (payload[i] << 7) | (crc.val >> 9);
    ++i;
    packed_payload[i+2] = (crc.val >> 1 & 0x80 ) | (crc.val >> 1 & 0x7F);
    ++i;
    packed_payload[i+2] = (crc.val << 7);

    *len += 4;
}

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static s16 scale_channel(u8 ch, s16 destMin, s16 destMax)
{
    s32 chanval = Channels[ch];
    s32 range = destMax - destMin;

    if      (chanval < CHAN_MIN_VALUE) chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE) chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)

static void send_packet(u8 bind)
{
    u8 packet_len = PACKET_SIZE;
    u8 packet_ori[8];
    if (bind)
    {
        // 4,5,6 = address fields
        // last field is checksum of address fields
        packet_ori[0] = 0x20;
        packet_ori[1] = 0x15;
        packet_ori[2] = 0x05;
        packet_ori[3] = 0x06;
        packet_ori[4] = rx_tx_addr[0];
        packet_ori[5] = rx_tx_addr[1];
        packet_ori[6] = rx_tx_addr[2];
        packet_ori[7] = packet_ori[4] + packet_ori[5] + packet_ori[6];
    }
    else
    {
        // throt, yaw, pitch, roll, trims, flags/left button,00,right button
        //0-3 0x00-0x64
        //4 roll/pitch/yaw trims. cycles through one trim at a time - 0-40 trim1, 40-80 trim2, 80-C0 trim3 (center:  A0 20 60)
        //5 flags for throttle button, two buttons above throttle - def: 0x40
        //6 00 ??
        //7 checksum - add values in other fields 

        
        // Trims are usually done through the radio configuration but leaving the code here just in case...
        u8 trim_mod  = packet_count % 144;
        u8 trim_val  = 0;
        if (36 <= trim_mod && trim_mod < 72) // yaw
            trim_val  = 0x20; // don't modify yaw trim
        else
            if (108 < trim_mod && trim_mod) // pitch
                trim_val = 0xA0;
            else // roll
                trim_val = 0x60;

        packet_ori[0] = scale_channel(CHANNEL3,0,0x64); // throttle
        packet_ori[1] = scale_channel(CHANNEL4,0x64,0x0);   // rudder
        packet_ori[2] = scale_channel(CHANNEL2,0,0x64); // elevator
        packet_ori[3] = scale_channel(CHANNEL1,0x64,0);  // aileron
        packet_ori[4] = trim_val; // calculated above
        packet_ori[5] = FLAG_EXPERT | FLAG_RETURN
                      | GET_FLAG(CHANNEL_FLIP, FLAG_FLIP)
                      | GET_FLAG(CHANNEL_HEADLESS, FLAG_HEADLESS);
        if( GET_FLAG(CHANNEL_RTH, 1))
            packet_ori[5] &= ~FLAG_RETURN;
                      
        packet_ori[6] = 0x00;
        // calculate checksum
        u8 checksum = 0;
        for (int i = 0; i < 7; ++i)
            checksum += packet_ori[i];
        packet_ori[7] = checksum;

        packet_count++;
    }

    ssv_pack_dpl( (0 == bind) ? rx_tx_addr : bind_addr, hopping_frequency_no, &packet_len, packet_ori, packet);
    
    NRF24L01_WriteReg(NRF24L01_00_CONFIG,_BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no++]);
    hopping_frequency_no %= NUM_RF_CHANNELS;
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, packet_len);
    NRF24L01_WritePayload(packet, packet_len);
    NRF24L01_WritePayload(packet, packet_len);
    
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bind_addr, 5);
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowledgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x00);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00); // no retransmits
    NRF24L01_SetBitrate(NRF24L01_BR_250K);
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_Activate(0x73);                         // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);      // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
    
    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    dbgprintf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        dbgprintf("BK2421 detected\n");
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\x99\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xDF\x96\x82\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xD9\x96\x82\x1B", 4);
    } else {
        dbgprintf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back
}

u16 fq777_callback()
{
    if(bind_counter!=0)
    {
        send_packet(1);
        bind_counter--;
        if (bind_counter == 0)
        {
            NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
            PROTOCOL_SetBindState(0);
        }
    }
    else
        send_packet(0);
    return PACKET_PERIOD;
}

void initFQ777(void)
{
    u32 lfsr = 0xb2c54a2ful;
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    PROTOCOL_SetBindState(BIND_COUNT * PACKET_PERIOD / 1000);
    bind_counter = BIND_COUNT;
    packet_count=0;
    
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for (int i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif

    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);
        
    rx_tx_addr[0] = (lfsr >> 8) & 0xff;
    rx_tx_addr[1] = lfsr & 0xff;
    rx_tx_addr[2] = 0x00;
    rx_tx_addr[3] = 0xe7;
    rx_tx_addr[4] = 0x67;
    hopping_frequency_no=0;
    init();
    CLOCK_StartTimer(INITIAL_WAIT, fq777_callback);
}

const void *FQ777_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initFQ777(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initFQ777(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 10L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)10L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return fq777_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
