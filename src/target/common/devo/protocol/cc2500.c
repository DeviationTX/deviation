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

#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "config/tx.h"
#include "protocol/interface.h"

#ifdef PROTO_HAS_CC2500
//GPIOA.14
static void  CS_HI() {
    if (Transmitter.module_enable[A7105].port == 0xFFFFFFFF) {
        gpio_set(Transmitter.module_enable[PROGSWITCH].port, Transmitter.module_enable[PROGSWITCH].pin);
        for(int i = 0; i < 20; i++)
            asm volatile ("nop");
    } else {
        gpio_set(Transmitter.module_enable[CC2500].port, Transmitter.module_enable[CC2500].pin);
    }
}

static void CS_LO() {
    if (Transmitter.module_enable[A7105].port == 0xFFFFFFFF) {
        gpio_clear(Transmitter.module_enable[PROGSWITCH].port, Transmitter.module_enable[PROGSWITCH].pin);
        for(int i = 0; i < 20; i++)
            asm volatile ("nop");
    } else {
        gpio_clear(Transmitter.module_enable[CC2500].port, Transmitter.module_enable[CC2500].pin);
    }
}

void CC2500_WriteReg(u8 address, u8 data)
{
    CS_LO();
    spi_xfer(SPI2, address);
    spi_xfer(SPI2, data);
    CS_HI();
}

static void ReadRegisterMulti(u8 address, u8 data[], u8 length)
{
    unsigned char i;

    CS_LO();
    spi_xfer(SPI2, address);
    for(i = 0; i < length; i++)
    {
        data[i] = spi_xfer(SPI2, 0);
    }
    CS_HI();
}

u8 CC2500_ReadReg(u8 address)
{
    CS_LO();
    spi_xfer(SPI2, CC2500_READ_SINGLE | address);
    u8 data = spi_xfer(SPI2, 0);
    CS_HI();
    return data;
}

void CC2500_ReadData(u8 *dpbuffer, int len)
{
    ReadRegisterMulti(CC2500_3F_RXFIFO | CC2500_READ_BURST, dpbuffer, len);
}

void CC2500_Strobe(u8 state)
{
    CS_LO();
    spi_xfer(SPI2, state);
    CS_HI();
}


void CC2500_WriteRegisterMulti(u8 address, const u8 data[], u8 length)
{
    CS_LO();
    spi_xfer(SPI2, CC2500_WRITE_BURST | address);
    for(int i = 0; i < length; i++)
    {
        spi_xfer(SPI2, data[i]);
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
    if(mode == TX_EN) {
        CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F | 0x40);
        CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F);
    } else if (mode == RX_EN) {
        CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F);
        CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F | 0x40);
    } else {
        CC2500_WriteReg(CC2500_02_IOCFG0, 0x2F);
        CC2500_WriteReg(CC2500_00_IOCFG2, 0x2F);
    }
}
void CC2500_Reset()
{
    CC2500_Strobe(CC2500_SRES);
}
#endif
