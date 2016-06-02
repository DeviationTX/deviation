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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/cm3/cortex.h>
#include "common.h"

#if defined HAS_4IN1_DL_SUPPORT && HAS_4IN1_DL_SUPPORT

#define CS_HI() do { cm_enable_interrupts(); gpio_set(GPIOB, GPIO12); } while(0)
#define CS_LO() do { cm_disable_interrupts(); SPISwitch_UseFlashModule(); \
                     gpio_clear(GPIOB, GPIO12); } while(0)
#define SPI_FLASH SPI2

// Defaults for SST25B (Devo 10, 7E etc)
#ifndef SPIFLASH_USE_EWSR
#define SPIFLASH_USE_EWSR 1
#endif

#ifndef SPIFLASH_USE_GLOBAL_PROTECT
#define SPIFLASH_USE_GLOBAL_PROTECT 0x1C
#endif

#ifndef SPIFLASH_USE_AAI
#define SPIFLASH_USE_AAI 1
#endif

/*
 * SPI flash shares with proto (RF) SPI, so we initialize this once
 * here. We need only to raise baudrate to DIV_4 when use flash and
 * drop to DIV_16 when using radios. It is done in SPISwitch.
 */
void SPIFlash_Init()
{
    /* Enable SPI2 */
    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_SPI2EN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

    /* SCK, MOSI */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13 | GPIO15);
    /* MISO */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO14);

    /* Reset and CS */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO11 | GPIO12);
    gpio_set(GPIOB, GPIO11);
    gpio_set(GPIOB, GPIO12);


    /* Includes enable? */
    spi_init_master(SPI2, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_4,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI2);
    spi_set_nss_high(SPI2);
    spi_enable(SPI2);
    
    SPISwitch_Init();
}

static void SPIFlash_SetAddr(unsigned cmd, u32 address)
{
    CS_LO();
    spi_xfer(SPI_FLASH, cmd);
    spi_xfer(SPI_FLASH, (u8)(address >> 16));
    spi_xfer(SPI_FLASH, (u8)(address >>  8));
    spi_xfer(SPI_FLASH, (u8)(address));
}

/*
 *
 */
u32 SPIFlash_ReadID()
{
    u32 result;

    SPIFlash_SetAddr(0x90, 0);
    result  = (u8)spi_xfer(SPI_FLASH, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPI_FLASH, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPI_FLASH, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPI_FLASH, 0);
   
    CS_HI();
    return result;
}

void SPI_FlashBlockWriteEnable(unsigned enable)
{
    CS_LO();
#if defined SPIFLASH_USE_EWSR  && SPIFLASH_USE_EWSR 
    spi_xfer(SPI_FLASH, 0x50);
#else
    spi_xfer(SPI_FLASH, 0x06);
#endif
    CS_HI();

    CS_LO();
#if defined SPIFLASH_USE_GLOBAL_PROTECT  && SPIFLASH_USE_GLOBAL_PROTECT
    spi_xfer(SPI_FLASH, 0x01);
    spi_xfer(SPI_FLASH, enable ? 0 : SPIFLASH_USE_GLOBAL_PROTECT);
#else
    Not yet implemented case: SST26
#endif
    CS_HI();
}

/*
 *
 */
void WriteFlashWriteEnable()
{
    CS_LO();
    spi_xfer(SPI_FLASH, 0x06);
    CS_HI();
}
/*
 *
 */
void WriteFlashWriteDisable()
{
    CS_LO();
    spi_xfer(SPI_FLASH, 0x04);
    CS_HI();
}

/*
 * Disable SO as RY/BY# status during AAI
 */
void DisableHWRYBY()
{
#if defined SPIFLASH_USE_AAI && SPIFLASH_USE_AAI
    CS_LO();
    spi_xfer(SPI_FLASH, 0x80);
    CS_HI();
#endif
}
/*
 *
 */
void WaitForWriteComplete()
{
    unsigned sr;
    // We disable interrupts in SPI operation so we
    // need to periodically re-enable them.
    while(true) {
        int i;
        CS_LO();
        spi_xfer(SPI_FLASH, 0x05);
        for (i = 0; i < 100; ++i) {
            sr = spi_xfer(SPI_FLASH, 0x00);
            if (!(sr & 0x01)) break;
        }
        CS_HI();
        if (i < 100) break;
    }
}
/*
 *
 */
void SPIFlash_EraseSector(u32 sectorAddress)
{
    //printf("SPI erase sector, addr %06x\r\n", sectorAddress);
    WriteFlashWriteEnable();

    SPIFlash_SetAddr(0x20, sectorAddress);
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
    spi_xfer(SPI_FLASH, 0xC7);
    CS_HI();

    WaitForWriteComplete();
}


#if defined SPIFLASH_USE_AAI && SPIFLASH_USE_AAI

#ifdef SPIFLASH_AAI_AF
/*
 * Use '0xAF' command to write byte-sized data with auto-increment-address
 */
void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer)
{
    u32 i;

    DisableHWRYBY();

    if(!length) return; // just in case...


    //printf("SPI write fast mode, length %d\r\n", fast_write_length);
    WriteFlashWriteEnable();

    SPIFlash_SetAddr(0xAF, writeAddress);
    spi_xfer(SPI_FLASH, ~buffer[0]);
    CS_HI();

    WaitForWriteComplete();

    for(i=1;i<length;i++)
    {
        CS_LO();
        spi_xfer(SPI_FLASH, 0xAF);
        spi_xfer(SPI_FLASH, ~buffer[i]);
        CS_HI();

        WaitForWriteComplete();
    }
    WriteFlashWriteDisable();
}

#else // ! defined SPIFLASH_AAI_AF
/*
 * Use '0xAD' command to write word-sized data with auto-increment-address
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

        SPIFlash_SetAddr(0xAD, writeAddress);
        spi_xfer(SPI_FLASH, ~buffer[0]);
        spi_xfer(SPI_FLASH, ~buffer[1]);
        CS_HI();

        WaitForWriteComplete();

        for(i=2;i<fast_write_length;i+=2)
        {
            CS_LO();
            spi_xfer(SPI_FLASH, 0xAD);
            spi_xfer(SPI_FLASH, ~buffer[i]);
            spi_xfer(SPI_FLASH, ~buffer[i+1]);
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
#endif

#else // Use PAGE_PROG

void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer)
{
    u32 i;

    if(!length) return; // just in case...


    //printf("SPI write page_prog, addr %06x, length %d\r\n", writeAddress, length);
    WriteFlashWriteEnable();

    SPIFlash_SetAddr(0x02, writeAddress);
    for(i=0;i<length;i++)
    {
        spi_xfer(SPI_FLASH, (u8)~buffer[i]);
    }
    CS_HI();
    WaitForWriteComplete();

    WriteFlashWriteDisable();

#if 0    
    // Verify
    bool error = false;
    u32 errorAddress;
    u8 errorByte;
    SPIFlash_SetAddr(0x0b, writeAddress);
    spi_xfer(SPI_FLASH, 0);
    for(i=0;i<length;i++)
    {
        u8 readByte = (u8) ~spi_xfer(SPI_FLASH, 0);
        if (buffer[i] != readByte) {
            error = true;
            errorAddress = writeAddress + i;
            errorByte = readByte;
            break;
        }
    }
    CS_HI();
    if (error) {
        printf("SPI verify failed, addr %06X, err byte %02X, bytes written:\r\n", errorAddress, errorByte);
        u32 l = length;
        u32 j = 0;
        u32 lim = j + 32;
        while (l) {
            printf("%06X : ", writeAddress + j);
            for (; j < lim && l > 0; ++j, --l) {
                printf("%02X ", buffer[j]);
            }
            printf("\r\n");
            lim = j + 32;
        }
    }
#endif
}

#endif

void SPIFlash_WriteByte(u32 writeAddress, const unsigned byte) {
   DisableHWRYBY();
   WriteFlashWriteEnable();
   SPIFlash_SetAddr(0x02, writeAddress);
   spi_xfer(SPI_FLASH, (u8)(~byte));
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
//    printf("SPI read, length %d\r\n", length);
#if defined SPIFLASH_USE_FAST_READ && SPIFLASH_USE_FAST_READ
    SPIFlash_SetAddr(0x0b, readAddress);
    spi_xfer(SPI_FLASH, 0);
#else
    SPIFlash_SetAddr(0x03, readAddress);
#endif

    for(i=0;i<length;i++)
    {
        buffer[i] = ~spi_xfer(SPI_FLASH, 0);
    }

    CS_HI();
}

int SPIFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer)
{
    u32 i;
#if defined SPIFLASH_USE_FAST_READ && SPIFLASH_USE_FAST_READ
    SPIFlash_SetAddr(0x0b, readAddress);
    spi_xfer(SPI_FLASH, 0);
#else
    SPIFlash_SetAddr(0x03, readAddress);
#endif

    for(i=0;i<length;i++)
    {
        buffer[i] = ~spi_xfer(SPI_FLASH, 0);
        if (buffer[i] == '\n') {
            i++;
            break;
        }
    }

    CS_HI();
    return i;
}

#endif // defined HAS_4IN1_DL_SUPPORT && HAS_4IN1_DL_SUPPORT
