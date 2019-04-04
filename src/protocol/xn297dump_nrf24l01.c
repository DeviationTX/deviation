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
#include "config/model.h"
#include "rftools.h"

#ifdef PROTO_HAS_NRF24L01


#ifdef MODULAR
    struct Xn297dump xn297dump;
#endif

#define INITIAL_WAIT       500
#define ADDRESS_LENGTH     5
#define DUMP_PERIOD        100
#define PACKET_SIZE        32
#define CRC_LENGTH         2

// Bit vector from bit position
#define BV(bit) (1 << bit)

enum {
    XN297DUMP_CHANNEL_CHANGE = 0,
    XN297DUMP_DUMP,
    XN297DUMP_SYNC
};

static u8 phase, channel;

/*static u8 checksum()
{
    u8 sum = packet[0];
    for (int i = 1; i < PACKET_SIZE - 1; i++)
        sum += packet[i];
    return sum;
}
*/

static void check_rx(void)
{
    if (xn297dump.channel != channel) {
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, xn297dump.channel);
        channel = xn297dump.channel;
    }
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR)) {
        // Receive Data, decrypt payload and check crc
        // undo encryption for 5 byte adress and reverse packet order
        int i;
        u16 packet_crc = 0;
        u16 crc = 0xb5d2;
        NRF24L01_ReadPayload(xn297dump.packet, xn297dump.pkt_len);
        
        // unscramble address and reverse order
        for (i = 0; i < ADDRESS_LENGTH; i++) {
            crc = crc16_update(crc, xn297dump.packet[i], 8);
            xn297dump.packet[i] ^= xn297_scramble[i];
        }
        u8 buf[5];
        memcpy(buf, xn297dump.packet, ADDRESS_LENGTH);
        for (i = 0; i < ADDRESS_LENGTH; i++) {
            xn297dump.packet[i] = buf[ADDRESS_LENGTH-1-i];
        }

        // unscramble payload
        for (i = ADDRESS_LENGTH; i < xn297dump.pkt_len - CRC_LENGTH; i++) {
            crc = crc16_update(crc, xn297dump.packet[i], 8);
            xn297dump.packet[i] = bit_reverse(xn297dump.packet[i] ^ xn297_scramble[i]);
        }
        
        // check crc
        packet_crc = ((u16)(xn297dump.packet[xn297dump.pkt_len - 2]) << 8)
                   | ((u16)(xn297dump.packet[xn297dump.pkt_len - 1]) & 0xff);
        
        crc ^= xn297_crc_xorout_scrambled[xn297dump.pkt_len-(ADDRESS_LENGTH-2+CRC_LENGTH)];
        
        // debug
        //xn297dump.packet[27] = packet_crc >> 8;
        //xn297dump.packet[28] = packet_crc & 0xff;
        //xn297dump.packet[30] = crc >> 8;
        //xn297dump.packet[31] = crc & 0xff;
        //

        if (packet_crc == crc)
            xn297dump.crc_valid = 1;
        else
            xn297dump.crc_valid = 0;

        NRF24L01_WriteReg(NRF24L01_07_STATUS, 255);
        NRF24L01_FlushRx();
    }
}

static void xn297dump_init()
{
    u8 rx_addr[] = { 0x55, 0x0f, 0x71 };  // preamble is 0x550f71 for XN297

    NRF24L01_Initialize();
    NRF24L01_SetTxRxMode(RX_EN);

    NRF24L01_FlushRx();
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x03);            // Disable CRC check for dumps
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);             // No Auto Acknowldgement on all data pipes
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x01);          // 3 byte RX/TX address
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_addr, 3);     // set up RX address to xn297 preamble
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, xn297dump.pkt_len);
    NRF24L01_SetBitrate(NRF24L01_BR_1M);                    // 1Mbps
    NRF24L01_Activate(0x73);                                // Activate feature register
    NRF24L01_WriteReg(NRF24L01_1C_DYNPD, 0x00);             // Disable dynamic payload length on all pipes
    NRF24L01_WriteReg(NRF24L01_1D_FEATURE, 0x01);
    NRF24L01_Activate(0x73);
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, channel);             // Start dumping on channel 0x00
}



static u16 xn297dump_callback()
{
    switch (phase) {
    case XN297DUMP_CHANNEL_CHANGE:
        xn297dump.channel++;
        phase = XN297DUMP_DUMP;
        break;
    case XN297DUMP_DUMP:
            check_rx();
        break;
    case XN297DUMP_SYNC:
        break;
    }
    return DUMP_PERIOD;
}


static void initialize()
{
    CLOCK_StopTimer();
    xn297dump_init();
    phase = XN297DUMP_DUMP;
    CLOCK_StartTimer(INITIAL_WAIT, xn297dump_callback);
}

uintptr_t XN297Dump_Cmds(enum ProtoCmds cmd)
{
    switch (cmd) {
    case PROTOCMD_INIT:
        initialize();
        return 0;
    case PROTOCMD_DEINIT:
    case PROTOCMD_RESET:
        CLOCK_StopTimer();
        return (NRF24L01_Reset()? 1 : -1);
    case PROTOCMD_CHECK_AUTOBIND:
        return 1;     // always Autobind
    case PROTOCMD_BIND:
        initialize();
        return 0;
    case PROTOCMD_NUMCHAN:
        return NUM_CHANNELS;
    case PROTOCMD_DEFAULT_NUMCHAN:
        return NUM_CHANNELS;
    case PROTOCMD_CURRENT_ID:
    case PROTOCMD_GETOPTIONS:
        return 0;
    case PROTOCMD_TELEMETRYSTATE:
        return PROTO_TELEM_UNSUPPORTED;
    case PROTOCMD_CHANNELMAP:
        return UNCHG;
    default:
        break;
    }
    return 0;
}
#endif  // PROTO_HAS_NRF24L01
