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

// xn297 emulation

// setup CC2500 for XN297L @ 250 kbps emulation
void XN297L_Configure(u8 scramble_en, u8 crc_en)
{
    // Address Config = No address check
    // Base Frequency = 2399.999268
    // CRC Autoflush = false
    // CRC Enable = false
    // Carrier Frequency = 2399.999268
    // Channel Number = 0
    // Channel Spacing = 333.251953
    // Data Format = Normal mode
    // Data Rate = 249.939
    // Deviation = 126.953125
    // Device Address = 0
    // Manchester Enable = false
    // Modulated = true
    // Modulation Format = GFSK
    // Packet Length = 255
    // Packet Length Mode = Variable packet length mode. Packet length configured by the first byte after sync word
    // Preamble Count = 4
    // RX Filter BW = 203.125000
    // Sync Word Qualifier Mode = No preamble/sync
    // TX Power = 0
    // Whitening = false

    CC2500_Reset();
    CC2500_Strobe(CC2500_SIDLE);
    CC2500_WriteReg(CC2500_08_PKTCTRL0, 0x01);   // Packet Automation Control
    CC2500_WriteReg(CC2500_0B_FSCTRL1,  0x0A);   // Frequency Synthesizer Control
    CC2500_WriteReg(CC2500_0C_FSCTRL0,  0x00);   // Frequency Synthesizer Control
    CC2500_WriteReg(CC2500_0D_FREQ2,    0x5C);   // Frequency Control Word, High Byte
    CC2500_WriteReg(CC2500_0E_FREQ1,    0x4E);   // Frequency Control Word, Middle Byte
    CC2500_WriteReg(CC2500_0F_FREQ0,    0xC3);   // Frequency Control Word, Low Byte
    CC2500_WriteReg(CC2500_10_MDMCFG4,  0x8D);   // Modem Configuration
    CC2500_WriteReg(CC2500_11_MDMCFG3,  0x3B);   // Modem Configuration
    CC2500_WriteReg(CC2500_12_MDMCFG2,  0x10);   // Modem Configuration
    CC2500_WriteReg(CC2500_13_MDMCFG1,  0x23);   // Modem Configuration
    CC2500_WriteReg(CC2500_14_MDMCFG0,  0xA4);   // Modem Configuration
    CC2500_WriteReg(CC2500_15_DEVIATN,  0x62);   // Modem Deviation Setting
    CC2500_WriteReg(CC2500_18_MCSM0,    0x18);   // Main Radio Control State Machine Configuration
    CC2500_WriteReg(CC2500_19_FOCCFG,   0x1D);   // Frequency Offset Compensation Configuration
    CC2500_WriteReg(CC2500_1A_BSCFG,    0x1C);   // Bit Synchronization Configuration
    CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0xC7);   // AGC Control
    CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0x00);   // AGC Control
    CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xB0);   // AGC Control
    CC2500_WriteReg(CC2500_21_FREND1,   0xB6);   // Front End RX Configuration
    CC2500_WriteReg(CC2500_23_FSCAL3,   0xEA);   // Frequency Synthesizer Calibration
    CC2500_WriteReg(CC2500_25_FSCAL1,   0x00);   // Frequency Synthesizer Calibration
    CC2500_WriteReg(CC2500_26_FSCAL0,   0x11);   // Frequency Synthesizer Calibration

    XN297_SetScrambledMode(scramble_en);
    xn297_crc = crc_en;
}

void XN297L_SetTXAddr(const u8* addr, u8 len)
{
    if (len > 5) len = 5;
    if (len < 3) len = 3;
    xn297_addr_len = len;
    memcpy(xn297_tx_addr, addr, len);
}

void XN297L_WritePayload(u8* msg, u8 len)
{
    u8 buf[36];
    u8 count = _xn297_write_payload(msg, len, buf);
    // halt Tx/Rx
    CC2500_Strobe(CC2500_SIDLE);
    // flush tx FIFO
    CC2500_Strobe(CC2500_SFTX);
    // set cc2500 packet length
    CC2500_WriteReg(CC2500_3F_TXFIFO, count + 3);
    // XN297L preamble
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, (u8*)"\x71\x0f\x55", 3);
    // packet
    CC2500_WriteRegisterMulti(CC2500_3F_TXFIFO, buf, count);
    // transmit
    CC2500_Strobe(CC2500_STX);
}

void XN297L_SetChannel(u8 ch)
{
    if (ch > 85)
        ch = 85;
    // channel spacing is 333.25 MHz
    CC2500_WriteReg(CC2500_0A_CHANNR, ch * 3);
}

#endif
