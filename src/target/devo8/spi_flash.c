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

#define CS_HI() gpio_set(GPIOB, GPIO2)
#define CS_LO() gpio_clear(GPIOB, GPIO2)

/*
 *
 */
void SPIFlash_Init()
{
    /* Enable SPI1 */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_SPI1EN);
    /* Enable GPIOA */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

    /* CS */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);

    /* SCK, MOSI */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO7);
    /* MISO */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO6);
    CS_HI();
    /* Includes enable */
    spi_init_master(SPI1, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_4,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);

    spi_enable(SPI1);
}
/*
 *
 */
u32 SPIFlash_ReadID()
{
    u32 result;
    CS_LO();

    spi_xfer(SPI1, 0x90);
    spi_xfer(SPI1, 0x00);
    spi_xfer(SPI1, 0x00);
    spi_xfer(SPI1, 0x00); /* Mfr, Dev */
    result  = (u8)spi_xfer(SPI1, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPI1, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPI1, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPI1, 0);
   
    CS_HI();
    return result;
}

void SPI_FlashBlockWriteEnable(u8 enable)
{
    CS_LO();
    spi_xfer(SPI1, 0x50);
    CS_HI();
    CS_LO();
    spi_xfer(SPI1, 0x01);
    spi_xfer(SPI1, enable ? 0 : 0x38);
    CS_HI();
}

/*
 *
 */
void WriteFlashWriteEnable()
{
    CS_LO();
    spi_xfer(SPI1, 0x06);
    CS_HI();
}
/*
 *
 */
void WriteFlashWriteDisable()
{
    CS_LO();
    spi_xfer(SPI1, 0x04);
    CS_HI();
}
/*
 *
 */
void DisableHWRYBY()
{
    CS_LO();
    spi_xfer(SPI1, 0x80);
    CS_HI();
}
/*
 *
 */
void WaitForWriteComplete()
{
    u8 sr;
    CS_LO();
    spi_xfer(SPI1, 0x05);
    do
    {
        sr = spi_xfer(SPI1, 0x00);
    }while(sr & 0x01); 
    CS_HI();
}
/*
 *
 */
void SPIFlash_EraseSector(u32 sectorAddress)
{
    WriteFlashWriteEnable();

    CS_LO();
    spi_xfer(SPI1, 0x20);
    spi_xfer(SPI1, (u8)(sectorAddress >> 16));
    spi_xfer(SPI1, (u8)(sectorAddress >>  8));
    spi_xfer(SPI1, (u8)(sectorAddress));
    CS_HI();

    WaitForWriteComplete();
}
/*
 *
 */
void SPIFlash_BulkErase()
{
    printf("BulkErase...\n");

    WriteFlashWriteEnable();

    CS_LO();
    spi_xfer(SPI1, 0xC7);
    CS_HI();

    WaitForWriteComplete();
}
/*
 * Length should be multiple of 2
 */
void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer)
{
    u32 i;

    DisableHWRYBY();

    if(!length) return; // just in case...

    // Write single byte at the start, if writeAddress is odd

    if(writeAddress & 0x01) { 
        //printf("SPI write slow start\r\n");
        SPIFlash_WriteByte(writeAddress,buffer[0]);
        buffer++;
        writeAddress++;
        length--;       
    }
    
    // More than one byte left to write -> write even number of bytes in fast mode
    if(length>1) { 
        u32 fast_write_length=length&0xFFFFFFFE;
        
        //printf("SPI write fast mode, length %d\r\n", fast_write_length);
        WriteFlashWriteEnable();

        CS_LO();
        spi_xfer(SPI1, 0xAD);
        spi_xfer(SPI1, (u8)(writeAddress >> 16));
        spi_xfer(SPI1, (u8)(writeAddress >>  8));
        spi_xfer(SPI1, (u8)(writeAddress));
        spi_xfer(SPI1, ~buffer[0]);
        spi_xfer(SPI1, ~buffer[1]);
        CS_HI();

        WaitForWriteComplete();

        for(i=2;i<fast_write_length;i+=2)
        {
            CS_LO();
            spi_xfer(SPI1, 0xAD);
            spi_xfer(SPI1, ~buffer[i]);
            spi_xfer(SPI1, ~buffer[i+1]);
            CS_HI();

            WaitForWriteComplete();
        }
        WriteFlashWriteDisable();
        
        length-=fast_write_length;
        buffer+=fast_write_length;
        writeAddress+=fast_write_length;                
    }

    // zero or one bytes left to write now
    if(length) {
       //printf("SPI write slow finish\r\n");
       SPIFlash_WriteByte(writeAddress, buffer[0]);
       
    }

}

void SPIFlash_WriteByte(u32 writeAddress, const u8 byte) {
   DisableHWRYBY();
   WriteFlashWriteEnable();
   CS_LO();
   spi_xfer(SPI1, 0x02);
   spi_xfer(SPI1, (u8)(writeAddress >> 16));
   spi_xfer(SPI1, (u8)(writeAddress >>  8));
   spi_xfer(SPI1, (u8)(writeAddress));
   spi_xfer(SPI1, ~byte);
   CS_HI();
   WaitForWriteComplete();
   WriteFlashWriteDisable();

}



/*
 *
 */
void SPIFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer)
{
    u32 i;
    CS_LO();
    spi_xfer(SPI1, 0x03);
    spi_xfer(SPI1, (u8)(readAddress >> 16));
    spi_xfer(SPI1, (u8)(readAddress >>  8));
    spi_xfer(SPI1, (u8)(readAddress));

    for(i=0;i<length;i++)
    {
        buffer[i] = ~spi_xfer(SPI1, 0);
    }

    CS_HI();
}
