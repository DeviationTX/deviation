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
    #define NCC1701_Cmds PROTO_Cmds
    #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "telemetry.h"

#ifdef MODULAR
    //Some versions of gcc applythis to definitions, others to calls
    //So just use long_calls everywhere
    //#pragma long_calls_off
    extern unsigned _data_loadaddr;
    const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
    #define USE_FIXED_MFGID
    #define dbgprintf printf
#else
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define NCC_WRITE_WAIT      2000
#define INITIAL_WAIT        500
#define NCC_PACKET_INTERVAL 10333
#define NCC_TX_PACKET_LEN   16
#define NCC_RX_PACKET_LEN   13

enum {
    NCC_BIND_TX1=0,
    NCC_BIND_RX1,
    NCC_BIND_TX2,
    NCC_BIND_RX2,
    NCC_TX3,
    NCC_RX3,
};

static u8 rx_tx_addr[5];
static u8 rx_id[5];
static u8 packet[NCC_TX_PACKET_LEN];
static u8 hopping_frequency[6];
static u8 hopping_frequency_no;
static u8 phase;
static u8 tx_power;

static const u8 NCC_TX_DATA[][6]= {
    { 0x6D, 0x97, 0x04, 0x48, 0x43, 0x26 }, 
    { 0x35, 0x4B, 0x80, 0x44, 0x4C, 0x0B },
    { 0x50, 0xE2, 0x32, 0x2D, 0x4B, 0x0A },
    { 0xBF, 0x34, 0xF3, 0x45, 0x4D, 0x0D },
    { 0xDD, 0x7D, 0x5A, 0x46, 0x28, 0x23 },
    { 0xED, 0x19, 0x06, 0x2C, 0x4A, 0x09 },
    { 0xE9, 0xA8, 0x91, 0x2B, 0x49, 0x07 },
    { 0x66, 0x17, 0x7D, 0x48, 0x43, 0x26 },
    { 0xC2, 0x93, 0x55, 0x44, 0x4C, 0x0B },
};

// For code readability
enum {
    CHANNEL1 = 0,
    CHANNEL2,
    CHANNEL3,
    CHANNEL4,
    CHANNEL5
};

#define CHANNEL_WARP CHANNEL5

// Bit vector from bit position
#define BV(bit) (1 << bit)

static void NCC_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);

    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);     // 5-byte RX/TX address
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (u8*)"\xE7\xE7\xC7\xD7\x67",5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,    (u8*)"\xE7\xE7\xC7\xD7\x67",5);
    
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);            // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);             // No Auto Acknowledgment on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);         // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, NCC_RX_PACKET_LEN); // Enable rx pipe 0
    NRF24L01_SetBitrate(NRF24L01_BR_250K);                  // NRF24L01_BR_1M, NRF24L01_BR_2M, NRF24L01_BR_250K
    NRF24L01_SetPower(tx_power);
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)   // switch to TX mode and disable CRC
                                        | (1 << NRF24L01_00_CRCO)
                                        | (1 << NRF24L01_00_PWR_UP)
                                        | (0 << NRF24L01_00_PRIM_RX));
}

static const u8 NCC_xor[]={0x80, 0x44, 0x64, 0x75, 0x6C, 0x71, 0x2A, 0x36, 0x7C, 0xF1, 0x6E, 0x52, 0x09, 0x9D};
static void NCC_Crypt_Packet()
{
    u16 crc = 0;
    for(u8 i=0; i < NCC_TX_PACKET_LEN-2; i++)
    {
        packet[i] ^= NCC_xor[i];
        crc = crc16_update(crc, packet[i], 8);
    }
    crc ^= 0x60DE;
    packet[NCC_TX_PACKET_LEN-2] = crc >> 8;
    packet[NCC_TX_PACKET_LEN-1] = crc;
}

static u8 NCC_Decrypt_Packet()
{
    u16 crc = 0;
    for(u8 i=0; i < NCC_RX_PACKET_LEN-2; i++)
    {
        crc = crc16_update(crc, packet[i], 8);
        packet[i] ^= NCC_xor[i];
    }
    crc ^= 0xA950;
    if((crc >> 8) == packet[NCC_RX_PACKET_LEN-2] && (crc & 0xFF) == packet[NCC_RX_PACKET_LEN-1] )
    {// CRC match
        return 1;
    }
    return 0;
}

#define CHAN_RANGE (CHAN_MAX_VALUE - CHAN_MIN_VALUE)
static u8 scale_channel(u8 ch, u8 destMin, u8 destMax)
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
static void NCC_Write_Packet()
{
    packet[0] = 0xAA;
    packet[1] = rx_tx_addr[0];
    packet[2] = rx_tx_addr[1];
    packet[3] = rx_id[0];
    packet[4] = rx_id[1];
    packet[5] = scale_channel(CHANNEL3, 0, 0x3d); // throttle 00-3D
    packet[6] = scale_channel(CHANNEL2, 0, 0xff); // elevator, original: 61-80-9F but works with 00-80-FF
    packet[7] = scale_channel(CHANNEL1, 0xff, 0); // aileron, original: 61-80-9F but works with 00-80-FF
    packet[8] = scale_channel(CHANNEL4, 0xff, 0); // rudder, original: 61-80-9F but works with 00-80-FF
    packet[9] = rx_id[2];
    packet[10] = rx_id[3];
    packet[11] = rx_id[4];
    packet[12] = GET_FLAG(CHANNEL_WARP, 0x02);    // Warp:0x00 -> 0x02
    packet[13] = packet[5]+packet[6]+packet[7]+packet[8]+packet[12];
    if(phase == NCC_BIND_TX1)
    {
        packet[0] = 0xBB;
        packet[5] = 0x01;
        packet[6] = rx_tx_addr[2];
        memset((void *)(packet+7),0x55,7);
        hopping_frequency_no ^= 1;
    }
    else
    {
        hopping_frequency_no++;
        if(hopping_frequency_no > 2) hopping_frequency_no = 0;
    }
    // change frequency
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
    // switch to TX mode and disable CRC
    NRF24L01_SetTxRxMode(TXRX_OFF);
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
                                        | (1 << NRF24L01_00_CRCO)
                                        | (1 << NRF24L01_00_PWR_UP)
                                        | (0 << NRF24L01_00_PRIM_RX));
    // clear packet status bits and TX FIFO
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    // send packet
    NCC_Crypt_Packet();
    NRF24L01_WritePayload(packet,NCC_TX_PACKET_LEN);
    
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void update_telemetry()
{
    if(packet[6] != 0)
        Telemetry.value[TELEM_FRSKY_RSSI] = 0; // crash indicator
    else
        Telemetry.value[TELEM_FRSKY_RSSI] = 100;
    TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
}

MODULE_CALLTYPE
static u16 ncc1701_callback()
{
    switch(phase)
    {
        case NCC_BIND_TX1:
            if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR))
            { // RX fifo data ready
                NRF24L01_ReadPayload(packet, NCC_RX_PACKET_LEN);
                if(NCC_Decrypt_Packet() && packet[1]==rx_tx_addr[0] && packet[2]==rx_tx_addr[1])
                {
                    rx_id[0] = packet[3];
                    rx_id[1] = packet[4];
                    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);            // Clear data ready, data sent, and retransmit
                    phase = NCC_BIND_TX2;
                    return NCC_PACKET_INTERVAL;
                }
            }
            NCC_Write_Packet();
            phase = NCC_BIND_RX1;
            return NCC_WRITE_WAIT;
        case NCC_BIND_RX1:
            // switch to RX mode and disable CRC
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(RX_EN);
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
                                                | (1 << NRF24L01_00_CRCO)
                                                | (1 << NRF24L01_00_PWR_UP)
                                                | (1 << NRF24L01_00_PRIM_RX));
            NRF24L01_FlushRx();
            phase = NCC_BIND_TX1;
            return NCC_PACKET_INTERVAL - NCC_WRITE_WAIT;
        case NCC_BIND_TX2:
            if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR))
            { // RX fifo data ready
                NRF24L01_ReadPayload(packet, NCC_RX_PACKET_LEN);
                if(NCC_Decrypt_Packet() && packet[1]==rx_tx_addr[0] && packet[2]==rx_tx_addr[1] && packet[3]==rx_id[0] && packet[4]==rx_id[1])
                {
                    rx_id[2] = packet[8];
                    rx_id[3] = packet[9];
                    rx_id[4] = packet[10];
                    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);            // Clear data ready, data sent, and retransmit
                    PROTOCOL_SetBindState(0);
                    phase = NCC_TX3;
                    return NCC_PACKET_INTERVAL;
                }
            }
            NCC_Write_Packet();
            phase = NCC_BIND_RX2;
            return NCC_WRITE_WAIT;
        case NCC_BIND_RX2:
            // switch to RX mode and disable CRC
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(RX_EN);
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
                                                | (1 << NRF24L01_00_CRCO)
                                                | (1 << NRF24L01_00_PWR_UP)
                                                | (1 << NRF24L01_00_PRIM_RX));
            NRF24L01_FlushRx();
            phase = NCC_BIND_TX2;
            return NCC_PACKET_INTERVAL - NCC_WRITE_WAIT;
        case NCC_TX3:
            if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR))
            { // RX fifo data ready
                NRF24L01_ReadPayload(packet, NCC_RX_PACKET_LEN);
                if(NCC_Decrypt_Packet() && packet[1]==rx_tx_addr[0] && packet[2]==rx_tx_addr[1] && packet[3]==rx_id[0] && packet[4]==rx_id[1])
                {
                    //Telemetry
                    //packet[5] and packet[7] roll angle
                    //packet[6] crash detect: 0x00 no crash, 0x02 crash
                    update_telemetry();
                }
            }
            NCC_Write_Packet();
            phase = NCC_RX3;
            return NCC_WRITE_WAIT;
        case NCC_RX3:
            // switch to RX mode and disable CRC
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(RX_EN);
            NRF24L01_WriteReg(NRF24L01_00_CONFIG, (0 << NRF24L01_00_EN_CRC)
                                                | (1 << NRF24L01_00_CRCO)
                                                | (1 << NRF24L01_00_PWR_UP)
                                                | (1 << NRF24L01_00_PRIM_RX));
            NRF24L01_FlushRx();
            phase = NCC_TX3;
            return NCC_PACKET_INTERVAL - NCC_WRITE_WAIT;
    }
    return 1000;
}

static void initialize_txid()
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

    // tx address
    for(i=0; i<4; i++)
        rx_tx_addr[i] = (lfsr >> (i*8)) & 0xff;
    rand32_r(&lfsr, 0);
    rx_tx_addr[4] = lfsr & 0xff;
}

static void initNCC(void)
{
    CLOCK_StopTimer();
    PROTOCOL_SetBindState(0xFFFFFFFF);
    
    // Load TX data
    initialize_txid();
    u8 rand = rx_tx_addr[3]%9;
    for(u8 i=0; i<3; i++)
    {
        rx_tx_addr[i] = NCC_TX_DATA[rand][i];
        hopping_frequency[i] = NCC_TX_DATA[rand][i+3];
    }

    // RX data is acquired during bind
    rx_id[0]=0x00;
    rx_id[1]=0x00;
    rx_id[2]=0x20;
    rx_id[3]=0x20;
    rx_id[4]=0x20;

    hopping_frequency[4]=0x08;  // bind channel 1
    hopping_frequency[5]=0x2A;  // bind channel 2
    hopping_frequency_no=4;     // start with bind
    tx_power = Model.tx_power;
    NCC_init();
    phase=NCC_BIND_TX1;
    
    CLOCK_StartTimer(INITIAL_WAIT, ncc1701_callback);
}

const void *NCC1701_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initNCC(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initNCC(); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 5L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)5L; // A,E,T,R,Warp
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return (void *)0L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE: return (void *)(long) TELEM_FRSKY;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
