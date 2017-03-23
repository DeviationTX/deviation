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
  #define DM002_Cmds PROTO_Cmds
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
    #define PACKET_PERIOD 150
    #define dbgprintf printf
#else
    #define BIND_COUNT 655
    #define PACKET_PERIOD 6100 // Timeout for callback in uSec
    #define DM002_BIND_COUNT 655 // 4 seconds
    //printf inside an interrupt handler is really dangerous
    //this shouldn't be enabled even in debug builds without explicitly
    //turning it on
    #define dbgprintf if(0) printf
#endif

#define DM002_PACKET_PERIOD     6100 // Timeout for callback in uSec
#define DM002_INITIAL_WAIT      500
#define DM002_PACKET_SIZE       12   // packets have 12-byte payload
#define DM002_RF_BIND_CHANNEL   0x27
#define DM002_NUM_RF_CHANNEL    4
#define DM002_ADDRESS_SIZE      5

static u8 packet[DM002_PACKET_SIZE];
static u8 hopping_frequency[DM002_NUM_RF_CHANNEL];
static u8 hopping_frequency_no;
static u8 rx_tx_addr[DM002_ADDRESS_SIZE];
static u8 tx_power;
static u8 phase;
static u8 packet_count;
static u16 bind_counter;


enum DM002_FLAGS {
    // flags going to packet[9]
    DM002_FLAG_FLIP     = 0x01, 
    DM002_FLAG_LED      = 0x02, 
    DM002_FLAG_MEDIUM   = 0x04, 
    DM002_FLAG_HIGH     = 0x08, 
    DM002_FLAG_RTH      = 0x10,
    DM002_FLAG_HEADLESS = 0x20,
    DM002_FLAG_CAMERA1  = 0x40,
    DM002_FLAG_CAMERA2  = 0x80,
};

// For code readability
enum {
    CHANNEL1 = 0, // Aileron
    CHANNEL2,     // Elevator
    CHANNEL3,     // Throttle
    CHANNEL4,     // Rudder
    CHANNEL5,     // Leds
    CHANNEL6,     // Flip
    CHANNEL7,     // Camera 1
    CHANNEL8,     // Camera 2
    CHANNEL9,     // Headless
    CHANNEL10,    // RTH
};

#define CHANNEL_LED         CHANNEL5
#define CHANNEL_FLIP        CHANNEL6
#define CHANNEL_CAMERA1     CHANNEL7
#define CHANNEL_CAMERA2     CHANNEL8
#define CHANNEL_HEADLESS    CHANNEL9
#define CHANNEL_RTH         CHANNEL10

enum {
    BIND,
    DATA
};

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

static void DM002_send_packet(u8 bind)
{
	memcpy(&packet[5],(uint8_t *)"\x00\x7F\x7F\x7F\x00\x00\x00",7);
	if(bind)
	{
		packet[0] = 0xAA;
		packet[1] = rx_tx_addr[0]; 
		packet[2] = rx_tx_addr[1];
		packet[3] = rx_tx_addr[2];
		packet[4] = rx_tx_addr[3];
	}
	else
	{
		packet[0]=0x55;
		// Throttle : 0 .. 200
		packet[1]=scale_channel(CHANNEL3,0,200);
		// Other channels min 0x57, mid 0x7F, max 0xA7
		packet[2] = scale_channel(CHANNEL4,0xA7,0x57);     // rudder
		packet[3] = scale_channel(CHANNEL1, 0xA7,0x57);  // aileron
		packet[4] = scale_channel(CHANNEL2, 0xA7, 0x57); // elevator
		// Features
		packet[9] =   DM002_FLAG_HIGH // high rate
                    | GET_FLAG(CHANNEL_FLIP,    DM002_FLAG_FLIP)
					| GET_FLAG(!CHANNEL_LED,    DM002_FLAG_LED)
					| GET_FLAG(CHANNEL_CAMERA1, DM002_FLAG_CAMERA1)
					| GET_FLAG(CHANNEL_CAMERA2, DM002_FLAG_CAMERA2)
					| GET_FLAG(CHANNEL_HEADLESS,DM002_FLAG_HEADLESS)
					| GET_FLAG(CHANNEL_RTH,     DM002_FLAG_RTH);
		// Packet counter
		if(packet_count&0x03)
		{
			packet_count++;
			hopping_frequency_no++;
			hopping_frequency_no&=4;
		}
		packet_count&=0x0F;
		packet[10] = packet_count;
		packet_count++;
	}
	//CRC
	for(uint8_t i=0;i<DM002_PACKET_SIZE-1;i++)
		packet[11]+=packet[i];
	
	// Power on, TX mode, 2byte CRC
	// Why CRC0? xn297 does not interpret it - either 16-bit CRC or nothing
	XN297_Configure(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
	if (bind)
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, DM002_RF_BIND_CHANNEL);
	else
		NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
	// clear packet status bits and TX FIFO
	NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
	NRF24L01_FlushTx();
	XN297_WritePayload(packet, DM002_PACKET_SIZE);

	if (tx_power != Model.tx_power) {
        //Keep transmit power updated
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void DM002_init()
{
    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(TX_EN);
	XN297_SetTXAddr((uint8_t *)"\x26\xA8\x67\x35\xCC", DM002_ADDRESS_SIZE);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0 only
    NRF24L01_SetBitrate(NRF24L01_BR_1M);             // 1Mbps
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

MODULE_CALLTYPE
static u16 dm002_callback()
{
    switch(phase) {
        case BIND:
            if(bind_counter == 0) {
                phase = DATA;
                XN297_SetTXAddr(rx_tx_addr, DM002_ADDRESS_SIZE);
            }
            else {
                DM002_send_packet(1);
                bind_counter--;
            }
            break;
            
        case DATA:
            DM002_send_packet(0);
            break;
    }
    return DM002_PACKET_PERIOD;
}

static void DM002_initialize_txid()
{
	// Not figured out tcix / rf channels yet
    // Only 2 IDs/RFs are available, model txid (even/odd) is used to switch between them
	if(Model.fixed_id & 1) { 
		memcpy(hopping_frequency,(uint8_t *)"\x34\x39\x43\x48",DM002_NUM_RF_CHANNEL);
        memcpy(rx_tx_addr,(uint8_t *)"\x47\x93\x00\x00\xD5",DM002_ADDRESS_SIZE);
    }
    else {
		memcpy(hopping_frequency,(uint8_t *)"\x35\x39\x3B\x3D",DM002_NUM_RF_CHANNEL);
        memcpy(rx_tx_addr,(uint8_t *)"\xAC\xA1\x00\x00\xD5",DM002_ADDRESS_SIZE);
    }
}

static void initialize()
{
    CLOCK_StopTimer();
    tx_power = Model.tx_power;
    DM002_initialize_txid();
    DM002_init();
    packet_count = 0;
    hopping_frequency_no = 0;
    bind_counter = DM002_BIND_COUNT;
    phase = BIND;
    CLOCK_StartTimer(DM002_INITIAL_WAIT, dm002_callback);
}

const void *DM002_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)1L; // always Autobind
        case PROTOCMD_BIND:  initialize(); return 0;
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