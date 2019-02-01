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
    #define LOLI_Cmds PROTO_Cmds
    #pragma long_calls
#endif

#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"  // for Transmitter

#ifdef MODULAR
    // Some versions of gcc apply this to definitions, others to calls
    // So just use long_calls everywhere
    // #pragma long_calls_off
    extern unsigned _data_loadaddr;
    const u32 protocol_type = (u32)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#define LOLI_PACKET_SIZE    11
#define LOLI_BIND_CHANNEL   33
#define LOLI_NUM_CHANNELS   5

static u8 packet[LOLI_PACKET_SIZE];
static u8 phase;
static u8 tx_power;
static u8 hopping_frequency[LOLI_NUM_CHANNELS];
static u8 rx_tx_addr[5];
static u8 hopping_frequency_no;
static u8 binding_count;
static const u8 bind_address[5] = {'L', 'O', 'V', 'E', '!'};

enum{
    BIND1,
    BIND2,
    BIND3,
    DATA1,
    DATA2
};

// Bit vector from bit position
#define BV(bit) (1 << bit)

static void init_RF()
{
    NRF24L01_Initialize();
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // enable rx data pipe 0
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0x00);    // No retransmit
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 66);   // start frequency
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, LOLI_PACKET_SIZE);  // RX FIFO size
    NRF24L01_SetBitrate(NRF24L01_BR_2M);
    tx_power = Model.tx_power;
    NRF24L01_SetPower(tx_power);
    NRF24L01_SetTxRxMode(TX_EN);
}

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

static void send_packet(u8 bind)
{
    u16 val;
    if (bind) {
        packet[0] = 0xa0;
        memcpy(&packet[1], hopping_frequency, 5);
        memcpy(&packet[6], rx_tx_addr, 5);
    } else {
        packet[0] = 0xa1;
        val = scale_channel(0, 0, 1023);
        packet[1] = val >> 2;
        packet[2] = val << 6;
        val = scale_channel(1, 0, 1023);
        packet[2]|= val >> 4;
        packet[3] = val << 4;
        val = scale_channel(2, 0, 1023);
        packet[3]|= val >> 6;
        packet[4] = val << 2;
        val = scale_channel(3, 0, 1023);
        packet[4]|= val >> 8;
        packet[5] = val & 0xff;
        val = scale_channel(4, 0, 1023);
        packet[6] = val >> 2;
        packet[7] = val << 6;
        val = scale_channel(5, 0, 1023);
        packet[7]|= val >> 4;
        packet[8] = val << 4;
        val = scale_channel(6, 0, 1023);
        packet[8]|= val >> 6;
        packet[9] = val << 2;
        val = scale_channel(7, 0, 1023);
        packet[9]|= val >> 8;
        packet[10]= val & 0xff;
        if (++hopping_frequency_no > LOLI_NUM_CHANNELS-1)
            hopping_frequency_no = 0;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
    }

    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
    NRF24L01_FlushTx();
    NRF24L01_WritePayload(packet, LOLI_PACKET_SIZE);

    // Keep transmit power updated
    if (tx_power != Model.tx_power) {
        tx_power = Model.tx_power;
        NRF24L01_SetPower(tx_power);
    }
}

static void update_telemetry()
{
    Telemetry.value[TELEM_FRSKY_RSSI] = packet[0]*2;
    TELEMETRY_SetUpdated(TELEM_FRSKY_RSSI);
    Telemetry.value[TELEM_FRSKY_VOLT1] = (packet[1] << 8) | packet[2];
    TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT1);
    Telemetry.value[TELEM_FRSKY_VOLT2] = (packet[3] << 8) | packet[4];
    TELEMETRY_SetUpdated(TELEM_FRSKY_VOLT2);
}

static u16 LOLI_callback()
{
    u16 delay = 0;
    switch (phase) {
        case BIND1:
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            // send bind packet
            send_packet(1);
            phase = BIND2;
            delay = 1000;
            break;
        case BIND2:
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_FlushRx();
            NRF24L01_SetTxRxMode(RX_EN);
            phase = BIND3;
            binding_count = 0;
            delay = 1000;
            break;
        case BIND3:
            // got bind response ?
            if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) {
                NRF24L01_ReadPayload(packet, LOLI_PACKET_SIZE);
                if (packet[0] == 'O' && packet[1] == 'K') {
                    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
                    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
                    PROTOCOL_SetBindState(0);
                    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);
                    NRF24L01_FlushRx();
                    phase = DATA1;
                    break;
                }
            }
            binding_count++;
            if (binding_count > 50) {
                phase = BIND1;
            } else {
                delay = 1000;
            }
            break;
        case DATA1:
            // got a telemetry packet ?
            if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) {  // RX fifo data ready
                NRF24L01_ReadPayload(packet, LOLI_PACKET_SIZE);
                update_telemetry();
            }
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_SetTxRxMode(TX_EN);
            // send data packet
            send_packet(0);
            phase = DATA2;
            delay = 2000;
            break;
        case DATA2:
            // switch to RX mode
            NRF24L01_SetTxRxMode(TXRX_OFF);
            NRF24L01_FlushRx();
            NRF24L01_SetTxRxMode(RX_EN);
            phase = DATA1;
            delay = 18000;
            break;
    }
    return delay;
}

static void init_txid()
{
    u32 lfsr = 0xb2c54a2ful;
#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
#endif
    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof(lfsr); ++i) rand32_r(&lfsr, 0);

    // tx id
    rx_tx_addr[0] = (lfsr >> 24) & 0xFF;
    rx_tx_addr[1] = ((lfsr >> 16) & 0xFF) % 0x30;
    rx_tx_addr[2] = (lfsr >> 8) & 0xFF;
    rx_tx_addr[3] = lfsr & 0xFF;
    rand32_r(&lfsr, 0);
    rx_tx_addr[4] = (lfsr >> 24) & 0xFF;

    // rf channels for data phase (manually set on "original" TX ?)
    for (u8 i=0; i < LOLI_NUM_CHANNELS; i++) {
        rand32_r(&lfsr, 0);
        hopping_frequency[i] = (lfsr & 0xff) % 84;  // 2400-2483 MHz
    }
}

static void initialize(u8 bind)
{
    CLOCK_StopTimer();
    init_txid();
    init_RF();
    if (bind) {
        phase = BIND1;
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, LOLI_BIND_CHANNEL);
        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, bind_address, 5);
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, bind_address, 5);
        PROTOCOL_SetBindState(0xFFFFFFFF);
    } else {
        NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
        phase = DATA1;
    }
    CLOCK_StartTimer(500, LOLI_callback);
}

const void *LOLI_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: return (void *)0L;
        case PROTOCMD_BIND:  initialize(1); return (void*)0L;
        case PROTOCMD_NUMCHAN: return (void *) 8L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((u32)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return (void*)0L;
        case PROTOCMD_TELEMETRYSTATE: return (void *)PROTO_TELEM_ON;
        case PROTOCMD_TELEMETRYTYPE: return (void *)(u32)TELEM_FRSKY;
        case PROTOCMD_CHANNELMAP: return AETRG;
        default: break;
    }
    return 0;
}

#endif
