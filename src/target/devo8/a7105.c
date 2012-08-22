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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "protocol/interface.h"

#define CS_HI() gpio_set(GPIOA, GPIO13)   
#define CS_LO() gpio_clear(GPIOA, GPIO13)

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
    for (i = 0; i < len; i++)
        spi_xfer(SPI2, dpbuffer[i]);
    CS_HI();

    A7105_WriteReg(0x0F, channel);

    CS_LO();
    spi_xfer(SPI2, A7105_TX);
    CS_HI();
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

void A7105_Strobe(enum A7105_State state)
{
    CS_LO();
    spi_xfer(SPI2, state);
    CS_HI();
}

void A7105_Initialize()
{
    /* Enable SPI2 */
    //rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_SPI2EN);
    /* Enable GPIOA */
    //rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);

    /* CS */
    AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
    CS_HI();

    A7105_Reset();
}
