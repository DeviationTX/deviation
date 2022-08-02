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
#include "config/tx.h"
#include "protocol/interface.h"
#include "protospi.h"

#ifdef PROTO_HAS_CC2500
//GPIOA.14
static void CS_HI() {
    PROTO_CS_HI(CC2500);
}

static void CS_LO() {
    PROTO_CS_LO(CC2500);
}

void CC2500_WriteReg(u8 address, u8 data)
{
    CS_LO();
    PROTOSPI_xfer(address);
    PROTOSPI_xfer(data);
    CS_HI();
}

void CC2500_ReadRegisterMulti(u8 address, u8 data[], u8 length)
{
    unsigned char i;

    CS_LO();
    PROTOSPI_xfer(CC2500_READ_BURST | address);
    for(i = 0; i < length; i++)
    {
        data[i] = PROTOSPI_xfer(0);
    }
    CS_HI();
}

u8 CC2500_ReadReg(u8 address)
{
    CS_LO();
    PROTOSPI_xfer(CC2500_READ_SINGLE | address);
    u8 data = PROTOSPI_xfer(0);
    CS_HI();
    return data;
}

void CC2500_ReadData(u8 *dpbuffer, int len)
{
    CC2500_ReadRegisterMulti(CC2500_3F_RXFIFO, dpbuffer, len);
}

void CC2500_Strobe(u8 state)
{
    CS_LO();
    PROTOSPI_xfer(state);
    CS_HI();
}


void CC2500_WriteRegisterMulti(u8 address, const u8 data[], u8 length)
{
    CS_LO();
    PROTOSPI_xfer(CC2500_WRITE_BURST | address);
    for(int i = 0; i < length; i++)
    {
        PROTOSPI_xfer(data[i]);
    }
    CS_HI();
}

void CC2500_WriteData(u8 *dpbuffer, u8 len)
{
    CC2500_Strobe(CC2500_SFTX);
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, dpbuffer, len);
    CC2500_Strobe(CC2500_STX);
}

void CC2500_SetTxRxMode(enum TXRX_State mode)
{
    // config-cc2500 = 0x01 for swapping GDO0 and GDO2.
    int R0 = CC2500_02_IOCFG0;
    int R2 = CC2500_00_IOCFG2;
    if (Transmitter.module_config[CC2500] == CC2500_REVERSE_GD02) {
      R0 = CC2500_00_IOCFG2;
      R2 = CC2500_02_IOCFG0;
    }

    if(mode == TX_EN) {
        CC2500_WriteReg(R2, 0x2F);
        CC2500_WriteReg(R0, 0x2F | 0x40);
    } else if (mode == RX_EN) {
        CC2500_WriteReg(R0, 0x2F);
        CC2500_WriteReg(R2, 0x2F | 0x40);
    } else {
        CC2500_WriteReg(R0, 0x2F);
        CC2500_WriteReg(R2, 0x2F);
    }
}

void CC2500_SetPower(int power)
{
    const unsigned char patable[8]=
    {
        0x60, // -30dbm
        0xA0, // -25dbm
        0x46, // -20dbm
        0x57, // -15dbm
        0x97, // -10dbm
        0xE7, //  -5dbm
        0xFE, //   0dbm
        0xFF  // 1.5dbm
    };
    if (power > 7)
        power = 7;
    CC2500_WriteReg(CC2500_3E_PATABLE,  patable[power]);
}
int CC2500_Reset()
{
    CC2500_Strobe(CC2500_SRES);
    usleep(1000);
    CC2500_SetTxRxMode(TXRX_OFF);
    return CC2500_ReadReg(CC2500_0E_FREQ1) == 0xC4;
}

// XN297 EMU

u8 xn297_crc;
u8 xn297_scramble_enabled;
u8 xn297_addr_len;
u8 xn297_tx_addr[5];
u8 xn297_rx_addr[5];

void XN297L_Configure(u8 scramble_en, u8 crc_en, u8 cc2500_packet_len)
{
    // Address Config = check for (0x55)
    // Base Frequency = 2400
    // CRC Autoflush = false
    // CRC Enable = false
    // Carrier Frequency = 2400
    // Channel Number = 0
    // Channel Spacing = 333.251953
    // Data Format = Normal mode
    // Data Rate = 249.939
    // Deviation = 126.953125
    // Device Address = 0
    // Manchester Enable = false
    // Modulated = true
    // Modulation Format = GFSK
    // Packet Length = fix len
    // Packet Length Mode = Variable packet length mode. Packet length configured by the first byte after sync word
    // Preamble Count = 1
    // RX Filter BW = 464kHz
    // !!! channel filter bandwidth should be selected so that the signal bandwidth occupies at most 80% of the channel filter bandwidth.
    // The channel centre tolerance due to crystal accuracy should also be subtracted from the signal bandwidth.
    // Sync Word Qualifier Mode = preamble (0x71 0x0F)
    // TX Power = 0
    // Whitening = false

    CC2500_Reset();
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_04_SYNC1,    0x71);   // Sync word, high byte  (Sync word = 0x71,0x0F,address=0x55)
    CC2500_WriteReg(CC2500_05_SYNC0,    0x0F);   // Sync word, low byte
    CC2500_WriteReg(CC2500_07_PKTCTRL1, 0x05);   // Packet Automation Control, address check true auto append RSSI & LQI
    CC2500_WriteReg(CC2500_06_PKTLEN,   cc2500_packet_len);  // Packet len, fix packet len
    CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x00);   // Packet Automation Control, fix packet len
    CC2500_WriteReg(CC2500_09_ADDR,     0x55);   // Set addr to 0x55  (Sync word = 0x71,0x0F,address=0x55)
    CC2500_WriteReg(CC2500_0B_FSCTRL1,  0x0A);   // Frequency Synthesizer Control (IF Frequency)
    CC2500_WriteReg(CC2500_0C_FSCTRL0,  0x00);   // Frequency Synthesizer Control
    CC2500_WriteReg(CC2500_0D_FREQ2,    0x5C);   // Frequency Control Word, High Byte
    CC2500_WriteReg(CC2500_0E_FREQ1,    0x4E);   // Frequency Control Word, Middle Byte
    CC2500_WriteReg(CC2500_0F_FREQ0,    0xC5);   // Frequency Control Word, Low Byte
    CC2500_WriteReg(CC2500_10_MDMCFG4,  0x3D);   // Modem Configuration  Set to 406kHz BW filter
    CC2500_WriteReg(CC2500_11_MDMCFG3,  0x3B);   // Modem Configuration
    CC2500_WriteReg(CC2500_12_MDMCFG2,  0x12);   // Modem Configuration  16/16 Sync Word detect
    CC2500_WriteReg(CC2500_13_MDMCFG1,  0x03);   // Modem Configuration
    CC2500_WriteReg(CC2500_14_MDMCFG0,  0xA4);   // Modem Configuration
    CC2500_WriteReg(CC2500_15_DEVIATN,  0x62);   // Modem Deviation Setting
    // CC2500_WriteReg(CC2500_16_MCSM2,    0x07);   // Main Radio Control State Machine Configuration
    // CC2500_WriteReg(CC2500_17_MCSM1,    0x30);   // Main Radio Control State Machine Configuration
    CC2500_WriteReg(CC2500_18_MCSM0,    0x28);   // Main Radio Control State Machine Configuration
    CC2500_WriteReg(CC2500_19_FOCCFG,   0x1D);   // Frequency Offset Compensation Configuration
    CC2500_WriteReg(CC2500_1A_BSCFG,    0x1C);   // Bit Synchronization Configuration
    CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xC7);   // AGC Control
    CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x00);   // AGC Control
    CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xB0);   // AGC Control
    CC2500_WriteReg(CC2500_21_FREND1,   0xB6);   // Front End RX Configuration
    CC2500_WriteReg(CC2500_23_FSCAL3,   0xEA);   // Frequency Synthesizer Calibration
    CC2500_WriteReg(CC2500_25_FSCAL1,   0x00);   // Frequency Synthesizer Calibration
    CC2500_WriteReg(CC2500_26_FSCAL0,   0x11);   // Frequency Synthesizer Calibration

    XN297L_SetScrambledMode(scramble_en);
    xn297_crc = crc_en;
}

void XN297L_SetTXAddr(const u8* addr, u8 len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    xn297_addr_len = len;
    memcpy(xn297_tx_addr, addr, len);
}

void XN297L_SetRXAddr(const u8* addr, u8 len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    xn297_addr_len = len;
    memcpy(xn297_rx_addr, addr, len);
}

void XN297L_SetChannel(u8 ch)
{
    if (ch > 85)
        ch = 85;
    // channel spacing is 333.25 MHz
    CC2500_WriteReg(CC2500_0A_CHANNR, ch * 3);
}

const u8 xn297_scramble[] = {
    0xE3, 0xB1, 0x4B, 0xEA, 0x85, 0xBC, 0xE5, 0x66,
    0x0D, 0xAE, 0x8C, 0x88, 0x12, 0x69, 0xEE, 0x1F,
    0xC7, 0x62, 0x97, 0xD5, 0x0B, 0x79, 0xCA, 0xCC,
    0x1B, 0x5D, 0x19, 0x10, 0x24, 0xD3, 0xDC, 0x3F,
    0x8E, 0xC5, 0x2F, 0xAA, 0x16, 0xF3, 0x95 };

const u16 xn297_crc_xorout_scrambled[] = {
    0x0000, 0x3448, 0x9BA7, 0x8BBB, 0x85E1, 0x3E8C,
    0x451E, 0x18E6, 0x6B24, 0xE7AB, 0x3828, 0x814B,
    0xD461, 0xF494, 0x2503, 0x691D, 0xFE8B, 0x9BA7,
    0x8B17, 0x2920, 0x8B5F, 0x61B1, 0xD391, 0x7401,
    0x2138, 0x129F, 0xB3A0, 0x2988, 0x23CA, 0xC0CB,
    0x0C6C, 0xB329, 0xA0A1, 0x0A16, 0xA9D0 };

const u16 xn297_crc_xorout[] = {
    0x0000, 0x3D5F, 0xA6F1, 0x3A23, 0xAA16, 0x1CAF,
    0x62B2, 0xE0EB, 0x0821, 0xBE07, 0x5F1A, 0xAF15,
    0x4F0A, 0xAD24, 0x5E48, 0xED34, 0x068C, 0xF2C9,
    0x1852, 0xDF36, 0x129D, 0xB17C, 0xD5F5, 0x70D7,
    0xB798, 0x5133, 0x67DB, 0xD94E, 0x0A5B, 0xE445,
    0xE6A5, 0x26E7, 0xBDAB, 0xC379, 0x8E20 };

const u16 xn297_crc_xorout_scrambled_enhanced[] = {
    0x0000, 0x7EBF, 0x3ECE, 0x07A4, 0xCA52, 0x343B,
    0x53F8, 0x8CD0, 0x9EAC, 0xD0C0, 0x150D, 0x5186,
    0xD251, 0xA46F, 0x8435, 0xFA2E, 0x7EBD, 0x3C7D,
    0x94E0, 0x3D5F, 0xA685, 0x4E47, 0xF045, 0xB483,
    0x7A1F, 0xDEA2, 0x9642, 0xBF4B, 0x032F, 0x01D2,
    0xDC86, 0x92A5, 0x183A, 0xB760, 0xA953 };

const u16 xn297_crc_xorout_enhanced[] = {
    0x0000, 0x8BE6, 0xD8EC, 0xB87A, 0x42DC, 0xAA89,
    0x83AF, 0x10E4, 0xE83E, 0x5C29, 0xAC76, 0x1C69,
    0xA4B2, 0x5961, 0xB4D3, 0x2A50, 0xCB27, 0x5128,
    0x7CDB, 0x7A14, 0xD5D2, 0x57D7, 0xE31D, 0xCE42,
    0x648D, 0xBF2D, 0x653B, 0x190C, 0x9117, 0x9A97,
    0xABFC, 0xE68E, 0x0DE7, 0x28A2, 0x1965 };


#if defined(__GNUC__) && defined(__ARM_ARCH_ISA_THUMB) && (__ARM_ARCH_ISA_THUMB == 2)
// rbit instruction works on cortex m3
u32 __RBIT_(u32 in)
{
    u32 out = 0;
    __asm volatile ("rbit %0, %1" : "=r" (out) : "r" (in) );
    return(out);
}

u8 bit_reverse(u8 b_in)
{
    return __RBIT_( (unsigned int) b_in)>>24;
}
#else
u8 bit_reverse(u8 b_in)
{
    u8 b_out = 0;
    for (int i = 0; i < 8; ++i) {
        b_out = (b_out << 1) | (b_in & 1);
        b_in >>= 1;
    }
    return b_out;
}
#endif

const u16 polynomial = 0x1021;

u16 crc16_update(u16 crc, u8 a, u8 bits)
{
    crc ^= a << 8;
    while (bits--) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ polynomial;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

void XN297L_SetScrambledMode(const u8 mode)
{
    xn297_scramble_enabled = mode;
}

u8 _xn297l_write_payload(const u8* msg, u8 len, u8* out)
{
    u8 last = 0;
    u8 i;

    for (i = 0; i < xn297_addr_len; ++i) {
        out[last] = xn297_tx_addr[xn297_addr_len-i-1];
        if (xn297_scramble_enabled)
            out[last] ^= xn297_scramble[i];
        last++;
    }

    for (i = 0; i < len; ++i) {
        // bit-reverse bytes in packet
        u8 b_out = bit_reverse(msg[i]);
        out[last] = b_out;
        if (xn297_scramble_enabled)
            out[last] ^= xn297_scramble[xn297_addr_len+i];
        last++;
    }

    if (xn297_crc) {
        u16 crc = 0xb5d2;
        for (i = 0; i < last; ++i) {
            crc = crc16_update(crc, out[i], 8);
        }
        if (xn297_scramble_enabled)
            crc ^= xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len];
        else
            crc ^= xn297_crc_xorout[xn297_addr_len - 3 + len];
        out[last++] = crc >> 8;
        out[last++] = crc & 0xff;
    }
    return last;
}

u8 _xn297l_write_enhancedpayload(const u8* msg, u8 len, u8* out, u8 noack)
{
        u8 scramble_index = 0;
        u8 last = 0;
        static u8 pid = 0;
        u8 i;
        // address
        for ( i = 0; i < xn297_addr_len; ++i ) {
            out[last] = xn297_tx_addr[xn297_addr_len-i-1];
            if (xn297_scramble_enabled)
                out[last] ^= xn297_scramble[scramble_index++];
            last++;
            }
        // pcf
        out[last] = (len << 1) | (pid>>1);
        if (xn297_scramble_enabled)
            out[last] ^= xn297_scramble[scramble_index++];
        last++;
        out[last] = (pid << 7) | (noack << 6);
        // payload
        out[last]|= bit_reverse(msg[0]) >> 2;  // first 6 bit of payload
        if (xn297_scramble_enabled)
            out[last] ^= xn297_scramble[scramble_index++];

        for ( i = 0; i < len-1; ++i ) {
            last++;
            out[last] = (bit_reverse(msg[i]) << 6) | (bit_reverse(msg[i+1]) >> 2);
            if (xn297_scramble_enabled)
                out[last] ^= xn297_scramble[scramble_index++];
        }

        last++;
        out[last] = bit_reverse(msg[len-1]) << 6;  // last 2 bit of payload
        if (xn297_scramble_enabled)
            out[last] ^= xn297_scramble[scramble_index++] & 0xc0;

        // crc
        if (xn297_crc) {
            u16 crc = 0xb5d2;
            for ( i = 0; i < last; ++i)
                crc = crc16_update(crc, out[i], 8);
            crc = crc16_update(crc, out[last] & 0xc0, 2);

            if (xn297_scramble_enabled)
                crc ^= xn297_crc_xorout_scrambled_enhanced[xn297_addr_len - 3+len];
            else
                crc ^= xn297_crc_xorout_enhanced[xn297_addr_len - 3 + len];

            out[last++] |= (crc >> 8) >> 2;
            out[last++] = ((crc >> 8) << 6) | ((crc & 0xff) >> 2);
            out[last++] = (crc & 0xff) << 6;
        }
        pid++;
        pid &= 3;
    return last;
}

void XN297L_WritePayload(u8* msg, u8 len)
{
    u8 buf[32];
    u8 count = _xn297l_write_payload(msg, len, buf);
    // halt Tx/Rx
    CC2500_Strobe(CC2500_SIDLE);
    // flush tx FIFO
    CC2500_Strobe(CC2500_SFTX);
    // set cc2500 packet length
    // CC2500_WriteReg(CC2500_3F_TXFIFO, count+3);
    // XN297L preamble
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (u8*)"\x55", 1);    // preamble was writen in cc2500 sync word & preamble.
    // packet
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buf, count);
    // transmit
    CC2500_Strobe(CC2500_STX);
}

void XN297L_WriteEnhancedPayload(u8* msg, u8 len, u8 noack)
{
    u8 buf[32];
    u8 count = _xn297l_write_enhancedpayload(msg, len, buf, noack);
    // halt Tx/Rx
    CC2500_Strobe(CC2500_SIDLE);
    // flush tx FIFO
    CC2500_Strobe(CC2500_SFTX);
    // set cc2500 packet length
    // CC2500_WriteReg(CC2500_3F_TXFIFO, count + 3);
    // XN297L preamble
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (u8*)"\x55", 1);  // preamble was writen in cc2500 sync word & preamble.
    // packet
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buf, count);
    // transmit
    CC2500_Strobe(CC2500_STX);
}

u8 XN297L_ReadPayload(u8* msg, u8 len)  // just what it should be, no tested yeah.
{  // cc2500 receive 1byte address(0x55) + 5byte xnl297 address + 16byte payload +2byte crc
    u8 buf[32];
    u8 rx_fifo = CC2500_ReadReg(CC2500_3B_RXBYTES | CC2500_READ_BURST) & 0x7F;
    CC2500_ReadData(buf, rx_fifo);
    // Decode payload
    for (u8 i = 0; i < len; i++)
    {
        u8 b_in = buf[i+6];
        if (xn297_scramble_enabled)
            b_in ^= xn297_scramble[i+xn297_addr_len];
        msg[i] = bit_reverse(b_in);
    }
    if (!xn297_crc)
        return 1;   // No CRC so OK by default...

    // Calculate CRC
    u16 crc = 0xb5d2;
    // process address
    for (u8 i = 0; i < xn297_addr_len; ++i)
    {
        u8 b_in = xn297_rx_addr[xn297_addr_len-i-1];
        if (xn297_scramble_enabled)
            b_in ^=  xn297_scramble[i];
        crc = crc16_update(crc, b_in, 8);
    }
    // process payload
    for (u8 i = 0; i < len; ++i)
        crc = crc16_update(crc, buf[i+6], 8);
    // xorout
    if (xn297_scramble_enabled)
        crc ^= xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len];
    else
        crc ^= xn297_crc_xorout[xn297_addr_len - 3 + len];
    // test
    if ((crc >> 8) == buf[len+6] && (crc & 0xff) == buf[len+7])
        return 1;   // CRC  OK
    return 0;       // CRC NOK
}

u8 XN297L_ReadEnhancedPayload(u8* msg, u8 len)
{  // cc2500 receive 1byte preamble(0x55) + 5byte xnl297 address + 2byte pcf + 16byte payload +2byte crc +2byte rx_status
    u8 buffer[32];
    u8 pcf_size;  // pcf payload size
    CC2500_ReadData(buffer, len);
    len = len -2;   //  2byte append rx_status
    pcf_size = buffer[6];   // 0x55+5byte addr
    if (xn297_scramble_enabled)
        pcf_size ^= xn297_scramble[xn297_addr_len];
    pcf_size = pcf_size >> 1;
    for (u8 i=0; i < len-10; i++)  // no include preamble(1byte) xnl297 address(5byte) PCF(2byte) crc(2byte)
    {
        msg[i] = bit_reverse((buffer[i+7] << 2) | (buffer[i+8] >> 6));
        if (xn297_scramble_enabled)
            msg[i] ^= bit_reverse((xn297_scramble[xn297_addr_len+i+1] << 2) |
                                  (xn297_scramble[xn297_addr_len+i+2] >> 6));
    }

    if (!xn297_crc)
        return pcf_size;    // No CRC so OK by default...

    // Calculate CRC
    u16 crc = 0xb5d2;
    for (u8 i = 0; i < len-4; ++i)
        crc = crc16_update(crc, buffer[i+1], 8);
    crc = crc16_update(crc, buffer[len-3] & 0xc0, 2);
    // xorout
    if (xn297_scramble_enabled)
        crc ^= xn297_crc_xorout_scrambled_enhanced[xn297_addr_len-3+16];

    u16 crcxored = (buffer[len-3] << 10)|(buffer[len-2] << 2)|(buffer[len-1] >> 6);
    if (crc == crcxored)
        return buffer[len];    // CRC  OK, return RSSI
    return 0;               // CRC NOK
}

#endif
