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
#include "protocol/interface.h"

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

void CYRF_Reset()
{
    /* Reset the CYRF chip */
    RS_HI();
    Delay(50);
    RS_LO();
    Delay(50);
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

    CYRF_Reset();
}

u8 CYRF_MaxPower()
{
    return (*((u8*)0x08001007) == 0) ? CYRF_PWR_100MW : CYRF_PWR_10MW;
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
void CYRF_ConfigCRCSeed(u16 crc)
{
    CYRF_WriteRegister(0x15,crc & 0xff);
    CYRF_WriteRegister(0x16,crc >> 8);
}
/*
 * these are the recommended sop codes from Crpress
 * See "WirelessUSB LP/LPstar and PRoC LP/LPstar Technical Reference Manual"
 */
void CYRF_ConfigSOPCode(const u8 *sopcodes)
{
    int i;
    for(i = 7; i >= 0; i--)
        CYRF_WriteRegister(0x22, sopcodes[i]);
    //WriteRegisterMulti(0x22, sopcodes, 8);
}

void CYRF_ConfigDataCode(const u8 *datacodes, u8 len)
{
    int i;
    for(i = len - 1; i >= 0; i--)
        CYRF_WriteRegister(0x23, datacodes[i]);
    //WriteRegisterMulti(0x23, datacodes, len);
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
    CS_LO();
        spi_xfer(SPI2, 0xC1);
    spi_xfer(SPI2, 0x10);
    spi_xfer(SPI2, 0xC3);
    CS_HI();
    WriteRegisterMulti(0x20, dpbuffer, 16);
/*
    CYRF_WriteRegister(0x02, 0x40);
    WriteRegisterMulti(0x20, dpbuffer, 16);
    CYRF_WriteRegister(0x02, 0xBF);
*/
}

void CYRF_WritePreamble(u32 preamble)
{
    CYRF_WriteRegister(0x24, (preamble >> 16) & 0xff);
    CYRF_WriteRegister(0x24, (preamble >> 8) & 0xff);
    CYRF_WriteRegister(0x24, (preamble >> 0) & 0xff);
/*
    CS_LO();
    spi_xfer(SPI2, 0x80 | 0x24);
    spi_xfer(SPI2, preamble & 0xff);
    spi_xfer(SPI2, (preamble >> 8) & 0xff);
    spi_xfer(SPI2, (preamble >> 16) & 0xff);
    CS_HI();
*/
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

