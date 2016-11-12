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

#ifdef SPIFLASH_TYPE

#include "spi_flash.h"

//#define _SPI_CONCAT(x, y, z) x ## y ## z
//#define SPI_CONCAT(x, y, z)  _SPI_CONCAT(x, y, z)
//#define SPIx           SPI_CONCAT(SPI,                         _SPI_FLASH_PORT,)
//#define SPIxEN         SPI_CONCAT(RCC_APB2ENR_SPI,             _SPI_FLASH_PORT, EN)

#if _SPI_FLASH_PORT == 1
    #define SPIx        SPI1
    #define SPIxEN      RCC_APB2ENR_SPI1EN
    #define APB_SPIxEN  RCC_APB2ENR
#elif _SPI_FLASH_PORT == 2
    #define SPIx        SPI2
    #define SPIxEN      RCC_APB1ENR_SPI2EN
    #define APB_SPIxEN  RCC_APB1ENR
#endif


#ifndef HAS_4IN1_FLASH
    #define HAS_4IN1_FLASH 0
#endif

void CS_HI()
{
    if (HAS_4IN1_FLASH && _SPI_PROTO_PORT == _SPI_FLASH_PORT) {
        cm_enable_interrupts();
    }
    PORT_pin_set(FLASH_CSN_PIN);
}

void CS_LO()
{
    if (HAS_4IN1_FLASH && _SPI_PROTO_PORT == _SPI_FLASH_PORT) {
        cm_disable_interrupts();
        SPISwitch_UseFlashModule();
    }
    PORT_pin_clear(FLASH_CSN_PIN);
}

/*
 *
 */
void SPIFlash_Init()
{
    /* Enable SPIx */
    rcc_peripheral_enable_clock(&APB_SPIxEN,  SPIxEN);
    /* Enable GPIOA */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);

    PORT_mode_setup(FLASH_CSN_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    PORT_mode_setup(FLASH_SCK_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    PORT_mode_setup(FLASH_MOSI_PIN, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL);
    PORT_mode_setup(FLASH_MISO_PIN, GPIO_MODE_INPUT,         GPIO_CNF_INPUT_FLOAT);

    CS_HI();
    /* Includes enable */
    spi_init_master(SPIx, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_4,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1, 
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPIx);
    spi_set_nss_high(SPIx);

    spi_enable(SPIx);
    if (HAS_4IN1_FLASH && _SPI_FLASH_PORT == _SPI_PROTO_PORT) {
        SPISwitch_Init();
    }
#if 0 //4IN1DEBUG
    // This code is equivalent to using SPISwitch_Init
    static const struct mcu_pin FLASH_RESET_PIN ={GPIOB, GPIO11};
    PORT_mode_setup(FLASH_RESET_PIN,  GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL);
    // And this is equivalent of using SPISwitch_UseFlashModule in CS_LO
    u8 cmd = 4;
    PORT_pin_clear(FLASH_RESET_PIN);
    spi_xfer(SPIx, cmd);
    PORT_pin_set(FLASH_RESET_PIN);
#endif
    /* Detect flash memory here */
}

static void SPIFlash_SetAddr(unsigned cmd, u32 address)
{
    CS_LO();
    spi_xfer(SPIx, cmd);
    spi_xfer(SPIx, (u8)(address >> 16));
    spi_xfer(SPIx, (u8)(address >>  8));
    spi_xfer(SPIx, (u8)(address));
}

/*
 *
 */
u32 SPIFlash_ReadID()
{
    u32 result;

    SPIFlash_SetAddr(0x90, 0);
    result  = (u8)spi_xfer(SPIx, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPIx, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPIx, 0);
    result <<= 8;
    result |= (u8)spi_xfer(SPIx, 0);
   
    CS_HI();
    return result;
}

void SPI_FlashBlockWriteEnable(unsigned enable)
{
    //printf("SPI_FlashBlockWriteEnable: %d\n", enable);
    CS_LO();
    spi_xfer(SPIx, SPIFLASH_SR_ENABLE);
    CS_HI();
    CS_LO();
    if (SPIFLASH_PROTECT_MASK) {
        spi_xfer(SPIx, 0x01);
        spi_xfer(SPIx, enable ? 0 : SPIFLASH_PROTECT_MASK);
    } else {
        //Not yet implemented case: SST26
    }
    CS_HI();
}

/*
 *
 */
void WriteFlashWriteEnable()
{
    CS_LO();
    spi_xfer(SPIx, 0x06);
    CS_HI();
}
/*
 *
 */
void WriteFlashWriteDisable()
{
    CS_LO();
    spi_xfer(SPIx, 0x04);
    CS_HI();
}
/*
 *
 */
void DisableHWRYBY()
{
    CS_LO();
    spi_xfer(SPIx, 0x80);
    CS_HI();
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
        spi_xfer(SPIx, 0x05);
        for (i = 0; i < 100; ++i) {
            sr = spi_xfer(SPIx, 0x00);
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
    spi_xfer(SPIx, 0xC7);
    CS_HI();

    WaitForWriteComplete();
}

void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const u8 * buffer)
{
    u32 i = 0;
    if(!length) return; // just in case...

    if (SPIFLASH_USE_AAI)
        DisableHWRYBY();

    //printf("SPI write page_prog, addr %06x, length %d\r\n", writeAddress, length);
    WriteFlashWriteEnable();

    if (SPIFLASH_USE_AAI) {
        if (SPIFLASH_WRITE_SIZE == 2 && writeAddress & 0x01) {
            //printf("SPI write slow start(%08x, %d)\n", writeAddress, length);
            SPIFlash_WriteByte(writeAddress,buffer[0]);
            buffer++;
            writeAddress++;
            length--;
            if (length == 0)
                return;
            WriteFlashWriteEnable();
        }
        SPIFlash_SetAddr(SPIFLASH_WRITE_CMD, writeAddress);
        spi_xfer(SPIx, (u8)~buffer[i++]);
        if (SPIFLASH_WRITE_SIZE == 2) {
            spi_xfer(SPIx, i < length ? ~buffer[i++] : 0xff);
        }
    } else {
        SPIFlash_SetAddr(0x02, writeAddress);
    }
    while(i < length) {
        if (SPIFLASH_USE_AAI) {
            CS_HI();
            WaitForWriteComplete();
            CS_LO();
            spi_xfer(SPIx, SPIFLASH_WRITE_CMD);
            if (SPIFLASH_WRITE_SIZE == 2) {
                //Writing 0xff will have no effect even if there is already data at this address
                spi_xfer(SPIx, i < length ? ~buffer[i++] : 0xff);
            }
        }
        spi_xfer(SPIx, (u8)~buffer[i++]);
    }
    CS_HI();
    WaitForWriteComplete();

    WriteFlashWriteDisable();
}

void SPIFlash_WriteByte(u32 writeAddress, const unsigned byte) {
   if (SPIFLASH_USE_AAI)
       DisableHWRYBY();
   WriteFlashWriteEnable();
   SPIFlash_SetAddr(0x02, writeAddress);
   spi_xfer(SPIx, (u8)(~byte));
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
    if (SPIFLASH_FAST_READ) {
        SPIFlash_SetAddr(0x0b, readAddress);
        spi_xfer(SPIx, 0); // Dummy read
    } else {
        SPIFlash_SetAddr(0x03, readAddress);
    }

    for(i=0;i<length;i++)
    {
        buffer[i] = ~spi_xfer(SPIx, 0);
    }

    CS_HI();
}

int SPIFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer)
{
    u32 i;
    if (SPIFLASH_FAST_READ) {
        SPIFlash_SetAddr(0x0b, readAddress);
        spi_xfer(SPIx, 0); // Dummy read
    } else {
        SPIFlash_SetAddr(0x03, readAddress);
    }

    for(i=0;i<length;i++)
    {
        buffer[i] = ~spi_xfer(SPIx, 0);
        if (buffer[i] == '\n') {
            i++;
            break;
        }
    }

    CS_HI();
    return i;
}

void debug_spi_flash()
{
    u8 data[512];
    u8 check[512];
    unsigned i;
    memset(check, 0, sizeof(check));
    int start = 0x1000*SPIFLASH_SECTOR_OFFSET;

    for (i = 0; i < sizeof(data); i++) {
        data[i] = rand32();
    }
    WriteFlashWriteEnable();
    SPIFlash_EraseSector(start);
    SPIFlash_ReadBytes(start, 101, check);
    for (i = 0; i < 101; i++) {
        if (check[i] != 0) {
          printf("Failed to erase at %d (%02x)\n", i, check[i]);
          break;
        }
    }
    SPIFlash_WriteBytes(start, 101, data);
    SPIFlash_ReadBytes(start, 101, check);
    printf("--------1\n");
    for (i = 0; i < 101; i++) {
        if (data[i] != check[i])  {
            printf("%04x: %02x != %02x\n", i, data[i], check[i]);
        }
    }
    printf("--------2\n");
    SPIFlash_WriteBytes(start +101 , 100, data + 101);
    SPIFlash_ReadBytes(start  +101 , 100, check + 101);
    for (i = 100; i < 201; i++) {
        if (data[i] != check[i])  {
            printf("%04x: %02x != %02x\n", i, data[i], check[i]);
        }
    }
    SPIFlash_WriteBytes(start + 223, 33, data+223);
    SPIFlash_WriteBytes(start + 201, 22, data+201);
    SPIFlash_ReadBytes(start + 200,  56, check + 200);
    printf("--------3\n");
    for (i = 200; i < 256; i++) {
        if (data[i] != check[i])  {
            printf("%04x: %02x != %02x\n", i, data[i], check[i]);
        }
    }
    while(1) { if(PWR_CheckPowerSwitch()) PWR_Shutdown(); }
}
#endif //SPIFLASH_TYPE
