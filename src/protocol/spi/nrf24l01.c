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
  #define DEVO_Cmds PROTO_Cmds
  #pragma long_calls
#endif

#include "common.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include "protospi.h"

#ifdef PROTO_HAS_NRF24L01

/* Instruction Mnemonics */
#define R_REGISTER    0x00
#define W_REGISTER    0x20
#define REGISTER_MASK 0x1F
#define ACTIVATE      0x50
#define R_RX_PL_WID   0x60
#define R_RX_PAYLOAD  0x61
#define W_TX_PAYLOAD  0xA0
#define W_ACK_PAYLOAD 0xA8
#define FLUSH_TX      0xE1
#define FLUSH_RX      0xE2
#define REUSE_TX_PL   0xE3
//#define NOP           0xFF	// already defined as NRF24L01_FF_NOP in iface_nrf24l01.h

// Bit vector from bit position
#define BV(bit) (1 << bit)

//GPIOA.14

static u8 rf_setup;

static void CS_HI() {
    PROTO_CS_HI(NRF24L01);
}

static void CS_LO() {
    PROTO_CS_LO(NRF24L01);
}
void NRF24L01_Initialize()
{
    rf_setup = 0x0F;
    XN297_SetScrambledMode(XN297_SCRAMBLED);
}

u8 NRF24L01_WriteReg(u8 reg, u8 data)
{
    CS_LO();
    u8 res = PROTOSPI_xfer(W_REGISTER | (REGISTER_MASK & reg));
    PROTOSPI_xfer(data);
    CS_HI();
    return res;
}

u8 NRF24L01_WriteRegisterMulti(u8 reg, const u8 data[], u8 length)
{
    CS_LO();
    u8 res = PROTOSPI_xfer(W_REGISTER | ( REGISTER_MASK & reg));
    for (u8 i = 0; i < length; i++)
    {
        PROTOSPI_xfer(data[i]);
    }
    CS_HI();
    return res;
}

u8 NRF24L01_WritePayload(u8 *data, u8 length)
{
    CS_LO();
    u8 res = PROTOSPI_xfer(W_TX_PAYLOAD);
    for (u8 i = 0; i < length; i++)
    {
        PROTOSPI_xfer(data[i]);
    }
    CS_HI();
    return res;
}

u8 NRF24L01_ReadReg(u8 reg)
{
    CS_LO();
    PROTOSPI_xfer(R_REGISTER | (REGISTER_MASK & reg));
    u8 data = PROTOSPI_xfer(0xFF);
    CS_HI();
    return data;
}

u8 NRF24L01_ReadRegisterMulti(u8 reg, u8 data[], u8 length)
{
    CS_LO();
    u8 res = PROTOSPI_xfer(R_REGISTER | (REGISTER_MASK & reg));
    for(u8 i = 0; i < length; i++)
    {
        data[i] = PROTOSPI_xfer(0xFF);
    }
    CS_HI();
    return res;
}

u8 NRF24L01_ReadPayload(u8 *data, u8 length)
{
    CS_LO();
    u8 res = PROTOSPI_xfer(R_RX_PAYLOAD);
    for(u8 i = 0; i < length; i++)
    {
        data[i] = PROTOSPI_xfer(0xFF);
    }
    CS_HI();
    return res;
}

static u8 Strobe(u8 state)
{
    CS_LO();
    u8 res = PROTOSPI_xfer(state);
    CS_HI();
    return res;
}

u8 NRF24L01_FlushTx()
{
    return Strobe(FLUSH_TX);
}

u8 NRF24L01_FlushRx()
{
    return Strobe(FLUSH_RX);
}

u8 NRF24L01_GetStatus(void)
{
    return Strobe(NRF24L01_FF_NOP);
}

u8 NRF24L01_GetDynamicPayloadSize(void)
{
    CS_LO();
    PROTOSPI_xfer(R_RX_PL_WID);
    const u8 res = PROTOSPI_xfer(NRF24L01_FF_NOP);
    CS_HI();
    return res;
}

u8 NRF24L01_Activate(u8 code)
{
    CS_LO();
    u8 res = PROTOSPI_xfer(ACTIVATE);
    PROTOSPI_xfer(code);
    CS_HI();
    return res;
}

u8 NRF24L01_SetBitrate(u8 bitrate)
{
    // Note that bitrate 250kbps (and bit RF_DR_LOW) is valid only
    // for nRF24L01+. There is no way to programmatically tell it from
    // older version, nRF24L01, but the older is practically phased out
    // by Nordic, so we assume that we deal with with modern version.

    // Bit 0 goes to RF_DR_HIGH, bit 1 - to RF_DR_LOW
    rf_setup = (rf_setup & 0xD7) | ((bitrate & 0x02) << 4) | ((bitrate & 0x01) << 3);
    return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}

// Power setting is 0..3 for nRF24L01
// Claimed power amp for nRF24L01 from eBay is 20dBm.
//      Raw            w 20dBm PA
// 0 : -18dBm  (16uW)   2dBm (1.6mW)
// 1 : -12dBm  (60uW)   8dBm   (6mW)
// 2 :  -6dBm (250uW)  14dBm  (25mW)
// 3 :   0dBm   (1mW)  20dBm (100mW)
// So it maps to Deviation as follows
/*
TXPOWER_100uW  = -10dBm
TXPOWER_300uW  = -5dBm
TXPOWER_1mW    = 0dBm
TXPOWER_3mW    = 5dBm
TXPOWER_10mW   = 10dBm
TXPOWER_30mW   = 15dBm
TXPOWER_100mW  = 20dBm
TXPOWER_150mW  = 22dBm
*/
u8 NRF24L01_SetPower(u8 power)
{
    u8 nrf_power = 0;
    switch(power) {
        case TXPOWER_100uW: nrf_power = 0; break;
        case TXPOWER_300uW: nrf_power = 0; break;
        case TXPOWER_1mW:   nrf_power = 0; break;
        case TXPOWER_3mW:   nrf_power = 1; break;
        case TXPOWER_10mW:  nrf_power = 1; break;
        case TXPOWER_30mW:  nrf_power = 2; break;
        case TXPOWER_100mW: nrf_power = 3; break;
        case TXPOWER_150mW: nrf_power = 3; break;
        default:            nrf_power = 0; break;
    }
    // Power is in range 0..3 for nRF24L01
    rf_setup = (rf_setup & 0xF9) | ((nrf_power & 0x03) << 1);
    return NRF24L01_WriteReg(NRF24L01_06_RF_SETUP, rf_setup);
}
static void CE_lo()
{
#if HAS_MULTIMOD_SUPPORT
    PROTOCOL_SetSwitch(NRF24L01);
#endif
}
static void CE_hi()
{
#if HAS_MULTIMOD_SUPPORT
    u8 en = SPI_ProtoGetPinConfig(NRF24L01, ENABLED_PIN);
    u8 csn = SPI_ProtoGetPinConfig(NRF24L01, CSN_PIN);
    SPI_ConfigSwitch(en | 0x0f, en | (0x0f ^ csn));
#endif
}

void NRF24L01_SetTxRxMode(enum TXRX_State mode)
{
    if(mode == TX_EN) {
        CE_lo();
        NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)    //reset the flag(s)
                                            | (1 << NRF24L01_07_TX_DS)
                                            | (1 << NRF24L01_07_MAX_RT));
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to TX mode
                                            | (1 << NRF24L01_00_CRCO)
                                            | (1 << NRF24L01_00_PWR_UP));
        usleep(130);
        CE_hi();
    } else if (mode == RX_EN) {
        CE_lo();
        NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);        // reset the flag(s)
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, 0x0F);        // switch to RX mode
        NRF24L01_WriteReg(NRF24L01_07_STATUS, (1 << NRF24L01_07_RX_DR)    //reset the flag(s)
                                            | (1 << NRF24L01_07_TX_DS)
                                            | (1 << NRF24L01_07_MAX_RT));
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)   // switch to RX mode
                                            | (1 << NRF24L01_00_CRCO)
                                            | (1 << NRF24L01_00_PWR_UP)
                                            | (1 << NRF24L01_00_PRIM_RX));
        usleep(130);
        CE_hi();
    } else {
        NRF24L01_WriteReg(NRF24L01_00_CONFIG, (1 << NRF24L01_00_EN_CRC)); //PowerDown
        CE_lo();
    }
}

int NRF24L01_Reset()
{
    NRF24L01_FlushTx();
    NRF24L01_FlushRx();
    u8 status1 = Strobe(NRF24L01_FF_NOP);
    u8 status2 = NRF24L01_ReadReg(NRF24L01_07_STATUS);
    NRF24L01_SetTxRxMode(TXRX_OFF);
#ifdef EMULATOR
    return 1;
#endif
    return (status1 == status2 && (status1 & 0x0f) == 0x0e);
}

// XN297 emulation layer

static u8  xn297_scramble_enabled;
static int xn297_addr_len;
static u8  xn297_tx_addr[5];
static u8  xn297_rx_addr[5];
static u8  xn297_crc = 0;

static const uint8_t xn297_scramble[] = {
    0xe3, 0xb1, 0x4b, 0xea, 0x85, 0xbc, 0xe5, 0x66,
    0x0d, 0xae, 0x8c, 0x88, 0x12, 0x69, 0xee, 0x1f,
    0xc7, 0x62, 0x97, 0xd5, 0x0b, 0x79, 0xca, 0xcc,
    0x1b, 0x5d, 0x19, 0x10, 0x24, 0xd3, 0xdc, 0x3f,
    0x8e, 0xc5, 0x2f};

FLASHWORDTABLE xn297_crc_xorout_scrambled[] = {
    0x0000, 0x3448, 0x9BA7, 0x8BBB, 0x85E1, 0x3E8C,
    0x451E, 0x18E6, 0x6B24, 0xE7AB, 0x3828, 0x814B,
    0xD461, 0xF494, 0x2503, 0x691D, 0xFE8B, 0x9BA7,
    0x8B17, 0x2920, 0x8B5F, 0x61B1, 0xD391, 0x7401,
    0x2138, 0x129F, 0xB3A0, 0x2988};

FLASHWORDTABLE xn297_crc_xorout[] = {
    0x0000, 0x3d5f, 0xa6f1, 0x3a23, 0xaa16, 0x1caf,
    0x62b2, 0xe0eb, 0x0821, 0xbe07, 0x5f1a, 0xaf15,
    0x4f0a, 0xad24, 0x5e48, 0xed34, 0x068c, 0xf2c9,
    0x1852, 0xdf36, 0x129d, 0xb17c, 0xd5f5, 0x70d7,
    0xb798, 0x5133, 0x67db, 0xd94e};


#if defined(__GNUC__) && defined(__ARM_ARCH_ISA_THUMB) && (__ARM_ARCH_ISA_THUMB==2)
// rbit instruction works on cortex m3
uint32_t __RBIT_(uint32_t in)
{
    uint32_t out=0;
    __asm volatile ("rbit %0, %1" : "=r" (out) : "r" (in) );
    return(out);
}

static uint8_t bit_reverse(uint8_t a)
{
    return __RBIT_( (unsigned int) a)>>24;
}
#else
static uint8_t bit_reverse(uint8_t b_in)
{
    uint8_t b_out = 0;
    for (int i = 0; i < 8; ++i) {
        b_out = (b_out << 1) | (b_in & 1);
        b_in >>= 1;
    }
    return b_out;
}
#endif

static const uint16_t polynomial = 0x1021;
static const uint16_t initial    = 0xb5d2;

static uint16_t crc16_update(uint16_t crc, unsigned char a, unsigned char bits)
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


void XN297_SetTXAddr(const u8* addr, int len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    u8 buf[] = { 0x55, 0x0F, 0x71, 0x0C, 0x00 }; // bytes for XN297 preamble 0xC710F55 (28 bit)
    xn297_addr_len = len;
    if (xn297_addr_len < 4) {
        for (int i = 0; i < 4; ++i) {
            buf[i] = buf[i+1];
        }
    }
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, buf, 5);
    // Receive address is complicated. We need to use scrambled actual address as a receive address
    // but the TX code now assumes fixed 4-byte transmit address for preamble. We need to adjust it
    // first. Also, if the scrambled address begings with 1 nRF24 will look for preamble byte 0xAA
    // instead of 0x55 to ensure enough 0-1 transitions to tune the receiver. Still need to experiment
    // with receiving signals.
    memcpy(xn297_tx_addr, addr, len);
}


void XN297_SetRXAddr(const u8* addr, int len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    u8 buf[] = { 0, 0, 0, 0, 0 };
    memcpy(buf, addr, len);

    memcpy(xn297_rx_addr, addr, len);
    for (int i = 0; i < xn297_addr_len; ++i) {
        buf[i] = xn297_rx_addr[i];
        if(xn297_scramble_enabled)
            buf[i] ^= xn297_scramble[xn297_addr_len-i-1];
    }
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, len-2);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, buf, 5);
}


void XN297_Configure(u8 flags)
{
    xn297_crc = !!(flags & BV(NRF24L01_00_EN_CRC));
    flags &= ~(BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO));
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, flags & 0xff);
}

void XN297_SetScrambledMode(const u8 mode)
{
    xn297_scramble_enabled = mode;
}

u8 XN297_WritePayload(u8* msg, int len)
{
    u8 packet[32];
    u8 res;

    int last = 0;
    if (xn297_addr_len < 4) {
        // If address length (which is defined by receive address length)
        // is less than 4 the TX address can't fit the preamble, so the last
        // byte goes here
        packet[last++] = 0x55;
    }
    for (int i = 0; i < xn297_addr_len; ++i) {
        packet[last] = xn297_tx_addr[xn297_addr_len-i-1];
        if(xn297_scramble_enabled)
            packet[last] ^= xn297_scramble[i];
        last++;
    }

    for (int i = 0; i < len; ++i) {
        // bit-reverse bytes in packet
        u8 b_out = bit_reverse(msg[i]);
        packet[last] = b_out;
        if(xn297_scramble_enabled)
            packet[last] ^= xn297_scramble[xn297_addr_len+i];
        last++;
    }
    if (xn297_crc) {
        int offset = xn297_addr_len < 4 ? 1 : 0;
        u16 crc = initial;
        for (int i = offset; i < last; ++i) {
            crc = crc16_update(crc, packet[i], 8);
        }
        if(xn297_scramble_enabled)
            crc ^= pgm_read_word(&xn297_crc_xorout_scrambled[xn297_addr_len - 3 + len]);
        else
            crc ^= pgm_read_word(&xn297_crc_xorout[xn297_addr_len - 3 + len]);
        packet[last++] = crc >> 8;
        packet[last++] = crc & 0xff;
    }
    res = NRF24L01_WritePayload(packet, last);
    return res;
}

u8 XN297_WriteEnhancedPayload(u8* msg, int len, int noack, u16 crc_xorout)
{
    u8 packet[32];
    u8 scramble_index=0;
    u8 res;
    int last = 0;
    static int pid=0;

    // address
    if (xn297_addr_len < 4) {
        // If address length (which is defined by receive address length)
        // is less than 4 the TX address can't fit the preamble, so the last
        // byte goes here
        packet[last++] = 0x55;
    }
    for (int i = 0; i < xn297_addr_len; ++i) {
        packet[last] = xn297_tx_addr[xn297_addr_len-i-1];
        if(xn297_scramble_enabled)
            packet[last] ^= xn297_scramble[scramble_index++];
        last++;
    }

    // pcf
    packet[last] = (len << 1) | (pid>>1);
    if(xn297_scramble_enabled)
        packet[last] ^= xn297_scramble[scramble_index++];
    last++;
    packet[last] = (pid << 7) | (noack << 6);

    // payload
    packet[last]|= bit_reverse(msg[0]) >> 2; // first 6 bit of payload
    if(xn297_scramble_enabled)
        packet[last] ^= xn297_scramble[scramble_index++];

    for (int i = 0; i < len-1; ++i) {
        last++;
        packet[last] = (bit_reverse(msg[i]) << 6) | (bit_reverse(msg[i+1]) >> 2);
        if(xn297_scramble_enabled)
            packet[last] ^= xn297_scramble[scramble_index++];
    }

    last++;
    packet[last] = bit_reverse(msg[len-1]) << 6; // last 2 bit of payload
    if(xn297_scramble_enabled)
            packet[last] ^= xn297_scramble[scramble_index++] & 0xc0;

    // crc
    if (xn297_crc) {
        int offset = xn297_addr_len < 4 ? 1 : 0;
        u16 crc = initial;
        for (int i = offset; i < last; ++i) {
            crc = crc16_update(crc, packet[i], 8);
        }
        crc = crc16_update(crc, packet[last] & 0xc0, 2);
        crc ^= crc_xorout;

        packet[last++] |= (crc >> 8) >> 2;
        packet[last++] = ((crc >> 8) << 6) | ((crc & 0xff) >> 2);
        packet[last++] = (crc & 0xff) << 6;
    }
    res = NRF24L01_WritePayload(packet, last);

    pid++;
    if(pid>3)
        pid=0;
    return res;
}

u8 XN297_ReadPayload(u8* msg, int len)
{
    // TODO: if xn297_crc==1, check CRC before filling *msg
    u8 res = NRF24L01_ReadPayload(msg, len);
    for(u8 i=0; i<len; i++) {
      msg[i] = bit_reverse(msg[i]);
      if(xn297_scramble_enabled)
        msg[i] ^= bit_reverse(xn297_scramble[i+xn297_addr_len]);
    }
    return res;
}

u8 XN297_ReadEnhancedPayload(u8* msg, int len)
{
    u8 buffer[32];
    u8 pcf_size; // pcf payload size
    NRF24L01_ReadPayload(buffer, len+2); // pcf + payload
    pcf_size = buffer[0];
    if(xn297_scramble_enabled)
        pcf_size ^= xn297_scramble[xn297_addr_len];
    pcf_size = pcf_size >> 1;
    for(int i=0; i<len; i++) {
        msg[i] = bit_reverse((buffer[i+1] << 2) | (buffer[i+2] >> 6));
        if(xn297_scramble_enabled)
            msg[i] ^= bit_reverse((xn297_scramble[xn297_addr_len+i+1] << 2) |
                                  (xn297_scramble[xn297_addr_len+i+2] >> 6));
    }
    return pcf_size;
}

// End of XN297 emulation

#endif // defined(PROTO_HAS_NRF24L01)
