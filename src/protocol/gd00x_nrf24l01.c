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
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter

#ifdef PROTO_HAS_NRF24L01

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define GD00X_BIND_COUNT      20
#define GD00X_PACKET_PERIOD   100
#define dbgprintf printf
#else
#define GD00X_BIND_COUNT    857 //3sec
#define GD00X_PACKET_PERIOD 3500 // Timeout for callback in uSec
//printf inside an interrupt handler is really dangerous
//this shouldn't be enabled even in debug builds without explicitly
//turning it on
#define dbgprintf if(0) printf
#endif

//#define FORCE_GD00X_ORIGINAL_ID

#define GD00X_INITIAL_WAIT 500
#define GD00X_RF_BIND_CHANNEL 2
#define GD00X_PAYLOAD_SIZE 15

#define GD00X_V2_BIND_PACKET_PERIOD	1700
#define GD00X_V2_RF_BIND_CHANNEL	0x43
#define GD00X_V2_PAYLOAD_SIZE	6

static const char * const gd00x_opts[] = {
  _tr_noop("Format"), "V1", "V2", NULL,
  NULL
};
enum {
    PROTOOPTS_FORMAT = 0,
    LAST_PROTO_OPT,
};
enum {
    FORMAT_V1 = 0,
    FORMAT_V2,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

static u8 tx_power;
static u8 packet[GD00X_PAYLOAD_SIZE];
static u8 hopping_frequency_no;
static u8 rx_tx_addr[5];
static u8 hopping_frequency[4];
static u16 bind_counter;
static u16 packet_period;
static u8 packet_count;
static u8 packet_length;
static u8 phase;
static u8 len;

enum{
    GD00X_BIND,
    GD00X_DATA
};

// flags going to packet[11]
#define GD00X_FLAG_DR       0x08
#define GD00X_FLAG_LIGHT    0x04

// flags going to packet[4]
#define	GD00X_V2_FLAG_DR	0x40
#define	GD00X_V2_FLAG_LIGHT	0x80

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5,
    CHANNEL6,
};

#define CHANNEL_LIGHT   CHANNEL5

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static u16 scale_channel(u8 ch, u16 destMin, u16 destMax)
{
    s32 chanval = Channels[ch];
    s32 range = (s32) destMax - (s32) destMin;

    if (chanval < CHAN_MIN_VALUE)
        chanval = CHAN_MIN_VALUE;
    else if (chanval > CHAN_MAX_VALUE)
        chanval = CHAN_MAX_VALUE;
    return (range * (chanval - CHAN_MIN_VALUE)) / CHAN_RANGE + destMin;
}

#define GET_FLAG(ch, mask) (Channels[ch] > 0 ? mask : 0)
static void GD00X_send_packet()
{
    static u8 prev_CH6=0;
    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_V1:
            packet[0] = (phase == GD00X_BIND) ? 0xAA : 0x55;
            memcpy(packet+1,rx_tx_addr,4);
            u16 channel=scale_channel(CHANNEL1, 2000, 1000);  // aileron
            packet[5 ] = channel;
            packet[6 ] = channel>>8;
            channel=scale_channel(CHANNEL3, 1000, 2000);  // throttle
            packet[7 ] = channel;
            packet[8 ] = channel>>8;
            // dynamically driven aileron trim
            channel=scale_channel(CHANNEL1, 1000, 2000);  // aileron
            packet[9 ] = channel;
            packet[10] = channel>>8;
            packet[11] = GD00X_FLAG_DR                      // Force high rate
                       | GET_FLAG(CHANNEL5, GD00X_FLAG_LIGHT);
            packet[12] = 0x00;
            packet[13] = 0x00;
            packet[14] = 0x00;
            break;
        case FORMAT_V2:
            if (phase == GD00X_BIND) {
                for(u8 i=0; i<5;i++)
                    packet[i]=rx_tx_addr[i];
            }
            else {
                packet[0]=scale_channel(CHANNEL3,0,100);	// throttle 0..100

                #define CHANNEL_MAX_100	1844
                #define CHANNEL_MIN_100	204
                #define GD00X_V2_DB_MIN 1024-40
                #define GD00X_V2_DB_MAX 1024+40
                // Deadband is needed on aileron
                u16 aileron=scale_channel(CHANNEL1, CHANNEL_MAX_100, CHANNEL_MIN_100);
                if (aileron>GD00X_V2_DB_MIN && aileron<GD00X_V2_DB_MAX)
                    packet[1]=0x20;	 // Send the channel centered
                else  // Ail:  0x3F..0x20..0x00
                    if (aileron>GD00X_V2_DB_MAX)
                        packet[1]=0x1F-((aileron-GD00X_V2_DB_MAX)*(0x20)/(CHANNEL_MAX_100+1-GD00X_V2_DB_MAX));	// 1F..00
                    else
                        packet[1]=0x3F-((aileron-CHANNEL_MIN_100)*(0x1F)/(GD00X_V2_DB_MIN-CHANNEL_MIN_100));	// 3F..21

                // Trims must be in a seperate channel for this model
                packet[2]=0x3F-(scale_channel(CHANNEL5,0,255)>>2);			// Trim: 0x3F..0x20..0x00

                u8 seq=((packet_count*3)/7)%5;
                packet[4]=seq | GD00X_V2_FLAG_DR;

                if (GET_FLAG(CHANNEL6,1) != prev_CH6)
                {  // LED switch is temporary
                    len=43;
                    prev_CH6=GET_FLAG(CHANNEL6,1);
                }
                if (len)
                {  // Send the light flag for a couple of packets
                    packet[4] |= GD00X_V2_FLAG_LIGHT;
                    len--;
                }

                packet[3]=(packet[0]+packet[1]+packet[2]+packet[4])^(rx_tx_addr[0]^rx_tx_addr[1]^rx_tx_addr[2]);

                if ((packet_count%12) == 0 )
                    hopping_frequency_no ^= 1;			// Toggle between the 2 frequencies
                packet_count++;
                if (packet_count>34) packet_count=0;		// Full period
                if ( seq == (((packet_count*3)/7)%5) )
                {
                    if (packet_period==2700)
                        packet_period=3000;
                    else
                        packet_period=2700;
                }
                else
                    packet_period=4300;
            }
            packet[5]='D';
            break;
    }

    // Power on, TX mode, CRC enabled
    XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    if (phase == GD00X_DATA) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
        if(Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1) {
            hopping_frequency_no++;
            hopping_frequency_no &= 3; // 4 RF channels
        }
    }

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    XN297_WritePayload(packet, packet_length);

    
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void GD00X_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_V1:
            XN297_SetTXAddr((u8*)"\xcc\xcc\xcc\xcc\xcc", 5);
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, GD00X_RF_BIND_CHANNEL);
            break;
        case FORMAT_V2:
            XN297_SetTXAddr((u8*)"GDKNx", 5);
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, GD00X_V2_RF_BIND_CHANNEL);
            break;
    }
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_250K);           // 250Kbps
    NRF24L01_SetPower(tx_power);
}

static void GD00X_initialize_txid()
{
    u32 lfsr = 0xb2c54a2ful;
    u8 i,j;
    
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    dbgprintf("Manufacturer id: ");
    for (i = 0; i < 12; ++i) {
        dbgprintf("%02X", var[i]);
        rand32_r(&lfsr, var[i]);
    }
    dbgprintf("\r\n");
#endif

    if (Model.fixed_id) {
       for (i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

    switch (Model.proto_opts[PROTOOPTS_FORMAT]) {
        case FORMAT_V1:
            // tx address
            for (i=0; i<2; i++)
                rx_tx_addr[i] = (lfsr >> (i*8)) & 0xff;
            
            rx_tx_addr[2]=0x12;
            rx_tx_addr[3]=0x13;
            
            u8 start=76+(rx_tx_addr[0]&0x03);
            for (i=0; i<4;i++)
                hopping_frequency[i]=start-(i<<1);
            
            #ifdef FORCE_GD00X_ORIGINAL_ID
                rx_tx_addr[0]=0x1F;
                rx_tx_addr[1]=0x39;
                rx_tx_addr[2]=0x12;
                rx_tx_addr[3]=0x13;
                for(i=0; i<4;i++)
                    hopping_frequency[i]=79-(i<<1);
            #endif
            break;
        case FORMAT_V2:
            //Generate 64 different IDs
            rx_tx_addr[1]=0x00;
            rx_tx_addr[2]=0x00;
            rx_tx_addr[1+((lfsr&0x10)>>4)]=lfsr&0x8F;
            rx_tx_addr[0]=0x65;
            rx_tx_addr[3]=0x95;
            rx_tx_addr[4]=0x47;	//'G'

            //hopping calculation
            hopping_frequency[0]=(0x15+(rx_tx_addr[0]^rx_tx_addr[1]^rx_tx_addr[2]^rx_tx_addr[3]))&0x1F;
            if( hopping_frequency[0] == 0x0F )
                hopping_frequency[0]=0x0E;
            else if( (hopping_frequency[0]&0xFE) == 0x10 )
                hopping_frequency[0]+=2;
            hopping_frequency[1]=0x20+hopping_frequency[0];

            #ifdef FORCE_GD00X_ORIGINAL_ID
                //ID 1
                rx_tx_addr[0]=0x65;
                rx_tx_addr[1]=0x00;
                rx_tx_addr[2]=0x00;
                rx_tx_addr[3]=0x95;
                rx_tx_addr[4]=0x47;	//'G'
                hopping_frequency[0]=0x05;
                hopping_frequency[1]=0x25;
                //ID 2
                rx_tx_addr[0]=0xFD;
                rx_tx_addr[1]=0x09;
                rx_tx_addr[2]=0x00;
                rx_tx_addr[3]=0x65;
                rx_tx_addr[4]=0x47;	//'G'
                hopping_frequency[0]=0x06;
                hopping_frequency[1]=0x26;
                //ID 3
                rx_tx_addr[0]=0x67;
                rx_tx_addr[1]=0x0F;
                rx_tx_addr[2]=0x00;
                rx_tx_addr[3]=0x69;
                rx_tx_addr[4]=0x47;	//'G'
                hopping_frequency[0]=0x16;
                hopping_frequency[1]=0x36;
            #endif
            break;
    }
}

static u16 GD00X_callback()
{
    if (phase == GD00X_BIND) {
        if (--bind_counter==0) {
            PROTOCOL_SetBindState(0);
            phase = GD00X_DATA;
        }
    }
    GD00X_send_packet();
    return packet_period;
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    packet_period=Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1?GD00X_PACKET_PERIOD:GD00X_V2_BIND_PACKET_PERIOD;
	packet_length=Model.proto_opts[PROTOOPTS_FORMAT]==FORMAT_V1?GD00X_PAYLOAD_SIZE:GD00X_V2_PAYLOAD_SIZE;
	packet_count=0;
    len=0;
    hopping_frequency_no = 0;
    bind_counter=GD00X_BIND_COUNT;
    phase = GD00X_BIND;
    PROTOCOL_SetBindState((GD00X_BIND_COUNT * packet_period)/1000);
    GD00X_initialize_txid();
    GD00X_init();
    CLOCK_StartTimer(GD00X_INITIAL_WAIT, GD00X_callback);
}

uintptr_t GD00X_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (NRF24L01_Reset() ? 1 : -1);
        case PROTOCMD_CHECK_AUTOBIND: return 1;
        case PROTOCMD_BIND:  initialize(); return 0;
        case PROTOCMD_NUMCHAN: return 5;
        case PROTOCMD_DEFAULT_NUMCHAN: return 5;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id;
        case PROTOCMD_GETOPTIONS: return (uintptr_t)gd00x_opts;
        case PROTOCMD_TELEMETRYSTATE: return PROTO_TELEM_UNSUPPORTED;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}


#endif
