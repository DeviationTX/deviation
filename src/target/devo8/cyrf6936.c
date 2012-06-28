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
#include "target.h"

#define CS_HI() gpio_set(GPIOB, GPIO12)   
#define CS_LO() gpio_clear(GPIOB, GPIO12)
#define RS_HI() gpio_set(GPIOB, GPIO11)
#define RS_LO() gpio_clear(GPIOB, GPIO11)

void Delay(uint32_t);

void CYRF_WriteRegister(u8 address, u8 data)
{
    CS_LO();
    spi_xfer(SPI2, 0x80 | address);
    spi_xfer(SPI2, data);
    CS_HI();
}

static void WriteRegisterMulti(u8 address, const u8 data[], u8 length)
{
    unsigned char i;

    CS_LO();
    spi_xfer(SPI2, 0x80 | address);
    for(i = 0; i < length; i++)
    {
        spi_xfer(SPI2, data[i]);
    }
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

u8 CYRF_ReadRegister(u8 address)
{
    u8 data;

    CS_LO();
    spi_xfer(SPI2, address);
    data = spi_xfer(SPI2, 0);
    CS_HI();
    return data;
}

void CYRF_Initialize()
{
    /* Enable SPI2 */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_SPI2EN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

    /* RESET, CS */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO11 | GPIO12);
    /* SCK, MOSI */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13 | GPIO15);
    /* MISO */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO14);
    CS_HI();
    RS_LO();

    /* Includes enable? */
    spi_init_master(SPI2, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_16,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);
    spi_enable(SPI2);

    /* Reset the CYRF chip */
    RS_HI();
    Delay(50);
    RS_LO();
    Delay(50);

    /* Initialise CYRF chip */
    CYRF_WriteRegister(0x1D, 0x39);
    CYRF_WriteRegister(0x03, 0x0B);
    CYRF_WriteRegister(0x06, 0x4A);
    CYRF_WriteRegister(0x0B, 0x00);
    CYRF_WriteRegister(0x0D, 0x04);
    CYRF_WriteRegister(0x0E, 0x20);
    CYRF_WriteRegister(0x10, 0xA4);
    CYRF_WriteRegister(0x11, 0x05);
    CYRF_WriteRegister(0x12, 0x0E);
    CYRF_WriteRegister(0x1B, 0x55);
    CYRF_WriteRegister(0x1C, 0x05);
    CYRF_WriteRegister(0x32, 0x3C);
    CYRF_WriteRegister(0x35, 0x14);
    CYRF_WriteRegister(0x39, 0x01);
    CYRF_WriteRegister(0x1E, 0x10);
    CYRF_WriteRegister(0x1F, 0x00);
    CYRF_WriteRegister(0x01, 0x10);
    CYRF_WriteRegister(0x0C, 0xC0);
    CYRF_WriteRegister(0x0F, 0x10);
    CYRF_WriteRegister(0x27, 0x02);
    CYRF_WriteRegister(0x28, 0x02);
    CYRF_WriteRegister(0x0F, 0x28);

}
/*
 *
 */
void CYRF_GetMfgData(u8 data[])
{
    /* Fuses power on */
    CYRF_WriteRegister(0x25, 0xFF);

    ReadRegisterMulti(0x25, data, 6);

    /* Fuses power off */
    CYRF_WriteRegister(0x25, 0x00); 
}
/*
 * 1 - Tx else Rx
 */
void CYRF_ConfigRxTx(u32 TxRx)
{
    if(TxRx)
    {
        CYRF_WriteRegister(0x0E,0x80);
        CYRF_WriteRegister(0x0F,0x2C);
    }
    else
    {
        CYRF_WriteRegister(0x0E,0x20);
        CYRF_WriteRegister(0x0F,0x28);
    }
}
/*
 *
 */
void CYRF_ConfigRFChannel(u8 ch)
{
    CYRF_WriteRegister(0x00,ch);
}
/*
 *
 */
void CYRF_ConfigCRCSeed(u8 crc)
{
    CYRF_WriteRegister(0x15,crc);
    CYRF_WriteRegister(0x16,crc);
}
/*
 *
 */
static const u8 sopcodes[] = {
    0x3C,0x37,0xCC,0x91,
    0xE2,0xF8,0xCC,0x91,
    0x9B,0xC5,0xA1,0x0F,
    0xAD,0x39,0xA2,0x0F,
    0xEF,0x64,0xB0,0x2A,
    0xD2,0x8F,0xB1,0x2A,
    0x66,0xCD,0x7C,0x50,
    0xDD,0x26,0x7C,0x50,
    0x5C,0xE1,0xF6,0x44,
    0xAD,0x16,0xF6,0x44,
    0x5A,0xCC,0xAE,0x46,
    0xB6,0x31,0xAE,0x46,
    0xA1,0x78,0xDC,0x3C,
    0x9E,0x82,0xDC,0x3C,
    0xB9,0x8E,0x19,0x74,
    0x6F,0x65,0x18,0x74,
    0xDF,0xB1,0xC0,0x49,
    0x62,0xDF,0xC1,0x49,
    0x97,0xE5,0x14,0x72,
    0x7F,0x1A,0x14,0x72
};

void CYRF_ConfigSOPCode(u32 idx)
{
    WriteRegisterMulti(0x22, &sopcodes[idx * 8], 8);
}
/*
 *
 */
void CYRF_StartReceive()
{
    CYRF_WriteRegister(0x05,0x87);
}

void CYRF_ReadDataPacket(u8 dpbuffer[])
{
    ReadRegisterMulti(0x21, dpbuffer, 0x10);
}

void CYRF_WriteDataPacket(u8 dpbuffer[])
{
    CYRF_WriteRegister(0x02, 0x40);
    WriteRegisterMulti(0x20, dpbuffer, 16);
    CYRF_WriteRegister(0x02, 0xBF);
}

u8 CYRF_ReadRSSI(u32 dodummyread)
{
    u8 result;
    if(dodummyread)
    {
        result = CYRF_ReadRegister(0x13);
    }
    result = CYRF_ReadRegister(0x13);
    if(result & 0x80)
    {
        result = CYRF_ReadRegister(0x13);
    }
    return (result & 0x0F);
}

