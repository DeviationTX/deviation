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
  #define ASSAN_Cmds PROTO_Cmds
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
#define dbgprintf printf
#else
#define dbgprintf if(0) printf
#endif

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define PACKET_SIZE     20
#define RF_BIND_CHANNEL 0x03
#define ADDRESS_LENGTH  4

enum {
    BIND0=0,
    BIND1,
    BIND2,
    DATA0,
    DATA1,
    DATA2,
    DATA3,
    DATA4,
    DATA5
};

u8 hopping_frequency[2];
u8 hopping_frequency_no;
u8 packet[PACKET_SIZE];
u8 tx_power;
u8 tx_address[2][ADDRESS_LENGTH];
u8 state;
u32 packet_count;

void init()
{
    NRF24L01_Initialize();
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x02);          // 4 bytes rx/tx address
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (u8 *)"\x80\x80\x80\xB8", ADDRESS_LENGTH);     // Bind address
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, (u8 *)"\x80\x80\x80\xB8", ADDRESS_LENGTH);  // Bind address
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);            // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);             // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);         // Enable data pipe 0 only
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, PACKET_SIZE);
    NRF24L01_SetPower(Model.tx_power);

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

void send_packet()
{
    u32 temp;
    for(u8 ch=0;ch<10;ch++)
    {
        temp=((s32)Channels[ch] * 0x1f1 / CHAN_MAX_VALUE + 0x5d9)<<3;
        packet[2*ch]=temp>>8;
        packet[2*ch+1]=temp;
    }
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);        // Clear data ready, data sent, and retransmit
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, PACKET_SIZE);

    // Check and adjust transmission power. We do this after
    // transmission to not bother with timeout after power
    // settings change -  we have plenty of time until next
    // packet.
    if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

#define WAIT 0x80

u16 ASSAN_callback()
{
    switch (state)
    {
    // Bind
        case BIND0:
            //Config RX @1M
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, RF_BIND_CHANNEL);
            NRF24L01_SetBitrate(NRF24L01_BR_1M);                    // 1Mbps
            NRF24L01_SetTxRxMode(RX_EN);
            state++;
        case BIND1:
            //Wait for receiver to send the frames
            if( NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR))
            { //Something has been received
                NRF24L01_ReadPayload(packet, PACKET_SIZE);
                if(packet[19]==0x13)
                { //Last frame received
                    state++;
                    state &= WAIT;
                    //Switch to TX
                    NRF24L01_SetTxRxMode(TXRX_OFF);
                    NRF24L01_SetTxRxMode(TX_EN);
                    //Prepare bind packet
                    memset(packet,0x05,PACKET_SIZE-5);
                    packet[15]=0x99;
                    for(u8 i=0;i<4;i++)
                        packet[16+i] = tx_address[1][ADDRESS_LENGTH-i-1];
                    packet_count=0;
                    return 10000;
                }
            }
            return 1000;
        case BIND2|WAIT:
            if(++packet_count == 27) // Wait 270ms in total...
            {
                packet_count = 0;
                state &= ~WAIT;
            }
            return 10000;
        case BIND2:
            // Send 20 packets
            packet_count++;
            if(packet_count==20)
                packet[15]=0x13;    // different value for last packet
            NRF24L01_WritePayload(packet, PACKET_SIZE);
            if(packet_count==20)
            {
                state++;
                state |= WAIT;
                packet_count = 0;
            }
            return 22520;
        case DATA0|WAIT:
            if(++packet_count == 217)
                state &= ~WAIT;
            return 10000;
    // Normal operation
        case DATA0:
            // Bind Done
            PROTOCOL_SetBindState(0);
            NRF24L01_SetBitrate(NRF24L01_BR_250K);                  // 250Kbps
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
        case DATA1:
        case DATA4:
            // Change ID and RF channel
            NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR,tx_address[hopping_frequency_no], ADDRESS_LENGTH);
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
            hopping_frequency_no^=0x01;
            state=DATA2;
            return 2000;
        case DATA2:
        case DATA3:
            send_packet();
            state++;    // DATA 3 or 4
            return 5000;
    }
    return 0;
}

static void initialize_txid()
{
    u8 freq=0,freq2;
    u32 lfsr = 0xb2c54a2ful;

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

    for(u8 i=0;i<4;i++)
    {
        u8 temp=lfsr & 0xff;
        tx_address[0][i]=temp;
        tx_address[1][i]=temp+1;
        freq+=temp;
    }

    // Main frequency
    freq=((freq%25)+2)<<1;
    if(freq&0x02)   freq|=0x01;
    hopping_frequency[0]=freq;
    // Alternate frequency has some random
    do
    {
        rand32_r(&lfsr, 0);
        freq2=lfsr%9;
        freq2+=freq*2-5;
    }
    while( (freq2>118) || (freq2<freq+1) || (freq2==2*freq) );
    hopping_frequency[1]=freq2;         // Add some randomness to the second channel
}

void initASSAN(u8 bind)
{
    CLOCK_StopTimer();
    initialize_txid();
    init();
    tx_power = Model.tx_power;
    hopping_frequency_no = 0;

    if(bind) {
        state=BIND0;
        PROTOCOL_SetBindState(0xffffffff);
    }
    else {
        state=DATA0;
        PROTOCOL_SetBindState(0);
    }
    CLOCK_StartTimer(50000, ASSAN_callback);
}

const void *ASSAN_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initASSAN(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L;
        case PROTOCMD_BIND:  initASSAN(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *) 10L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)10L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return (void*)0L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}

#endif