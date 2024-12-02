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
#endif
