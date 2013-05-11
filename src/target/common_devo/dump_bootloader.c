#include "common.h"

void dump_bootloader()
{
    LCD_PrintStringXY(40, 10, "Dumping");

    printf("Erase...\n");

    SPIFlash_EraseSector(0x1000*SPIFLASH_SECTOR_OFFSET + 0x0000);
    SPIFlash_EraseSector(0x1000*SPIFLASH_SECTOR_OFFSET + 0x1000);
    SPIFlash_EraseSector(0x1000*SPIFLASH_SECTOR_OFFSET + 0x2000);
    SPIFlash_EraseSector(0x1000*SPIFLASH_SECTOR_OFFSET + 0x3000);

    printf("Pgm 2\n");
    SPIFlash_WriteBytes(0x1000*SPIFLASH_SECTOR_OFFSET + 0x0000, 0x1000, (u8*)0x08000000);
    printf("Pgm 3\n");
    SPIFlash_WriteBytes(0x1000*SPIFLASH_SECTOR_OFFSET + 0x1000, 0x1000, (u8*)0x08001000);
    printf("Pgm 4\n");
    SPIFlash_WriteBytes(0x1000*SPIFLASH_SECTOR_OFFSET + 0x2000, 0x1000, (u8*)0x08002000);
    printf("Pgm 5\n");
    SPIFlash_WriteBytes(0x1000*SPIFLASH_SECTOR_OFFSET + 0x3000, 0x1000, (u8*)0x08003000);
    printf("Done\n");

    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Done");

    while(1)
    {
        if(PWR_CheckPowerSwitch())
        PWR_Shutdown();
    }
}
