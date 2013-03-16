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

#ifdef PROTO_HAS_A7105
//GPIOA.13
#define CS_HI() gpio_set(Transmitter.module_enable[A7105].port, Transmitter.module_enable[A7105].pin)
#define CS_LO() gpio_clear(Transmitter.module_enable[A7105].port, Transmitter.module_enable[A7105].pin)

void A7105_WriteReg(u8 address, u8 data)
{
    CS_LO();
    spi_xfer(SPI2, address);
    spi_xfer(SPI2, data);
    CS_HI();
}

u8 A7105_ReadReg(u8 address)
{
    u8 data;

    CS_LO();
    spi_xfer(SPI2, 0x40 | address);
    spi_disable(SPI2);
    spi_set_bidirectional_receive_only_mode(SPI2);
    spi_enable(SPI2);
    int i;
    for(i = 0; i < 10; i++)
       ;
    spi_disable(SPI2);
    data = spi_read(SPI2);
    CS_HI();
    spi_set_unidirectional_mode(SPI2);
    spi_enable(SPI2);
    return data;
}

void A7105_WriteData(u8 *dpbuffer, u8 len, u8 channel)
{
    int i;
    CS_LO();
    spi_xfer(SPI2, A7105_RST_WRPTR);
    spi_xfer(SPI2, 0x05);
    for (i = 0; i < len; i++)
        spi_xfer(SPI2, dpbuffer[i]);
    CS_HI();

    A7105_WriteReg(0x0F, channel);

    CS_LO();
    spi_xfer(SPI2, A7105_TX);
    CS_HI();
}
void A7105_ReadData(u8 *dpbuffer, u8 len)
{
    A7105_Strobe(0xF0); //A7105_RST_RDPTR
    for(int i = 0; i < len; i++)
        dpbuffer[i] = A7105_ReadReg(0x05);
/*
    CS_LO();
    spi_xfer(SPI2, 0x40 | 0x05);
    spi_disable(SPI2);
    spi_set_bidirectional_receive_only_mode(SPI2);
    spi_enable(SPI2);
    int i;
    for(i = 0; i < 10; i++)
       ;
    spi_disable(SPI2);
    for(i = 0; i < len; i++)
        dpbuffer[i] = spi_read(SPI2);
    CS_HI();
    spi_set_unidirectional_mode(SPI2);
    spi_enable(SPI2);
*/
    return;
}

void A7105_Reset()
{
    A7105_WriteReg(0x00, 0x00);
}
void A7105_WriteID(u32 id)
{
    CS_LO();
    spi_xfer(SPI2, 0x06);
    spi_xfer(SPI2, (id >> 24) & 0xFF);
    spi_xfer(SPI2, (id >> 16) & 0xFF);
    spi_xfer(SPI2, (id >> 8) & 0xFF);
    spi_xfer(SPI2, (id >> 0) & 0xFF);
    CS_HI();
}

void A7105_SetPower(int power)
{
    /*
    Power amp is ~+16dBm so:
    TXPOWER_100uW  = -23dBm == PAC=0 TBG=0
    TXPOWER_300uW  = -20dBm == PAC=0 TBG=1
    TXPOWER_1mW    = -16dBm == PAC=0 TBG=2
    TXPOWER_3mW    = -11dBm == PAC=0 TBG=4
    TXPOWER_10mW   = -6dBm  == PAC=1 TBG=5
    TXPOWER_30mW   = 0dBm   == PAC=2 TBG=7
    TXPOWER_100mW  = 1dBm   == PAC=3 TBG=7
    TXPOWER_150mW  = 1dBm   == PAC=3 TBG=7
    */
    u8 pac, tbg;
    switch(power) {
        case 0: pac = 0; tbg = 0; break;
        case 1: pac = 0; tbg = 1; break;
        case 2: pac = 0; tbg = 2; break;
        case 3: pac = 0; tbg = 4; break;
        case 4: pac = 1; tbg = 5; break;
        case 5: pac = 2; tbg = 7; break;
        case 6: pac = 3; tbg = 7; break;
        case 7: pac = 3; tbg = 7; break;
        default: pac = 0; tbg = 0; break;
    };
    A7105_WriteReg(0x28, (pac << 3) | tbg);
}

void A7105_Strobe(enum A7105_State state)
{
    CS_LO();
    spi_xfer(SPI2, state);
    CS_HI();
}

#pragma long_calls_off
#endif
