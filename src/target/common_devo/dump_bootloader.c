#include "common.h"

//UART bootloader dump
void dump_bootloader()
{
    u32 *ptr = (u32*)0x08000000;
    for(int j = 0; j < 3; j++) {
        printf("\nAddress: 0x%08x\n", (u32)ptr);
        for(int i = 0; i < 0x1000 / 4; i++) {
            u32 b = *ptr;
            printf("%02x", 0xff & (b >> 0));
            printf("%02x", 0xff & (b >> 8));
            printf("%02x", 0xff & (b >> 16));
            printf("%02x", 0xff & (b >> 24));
            ptr++;
        }
    }

    while(1)
    {
        if(PWR_CheckPowerSwitch())
        PWR_Shutdown();
    }
}
#if 0
//SPIFlash bootloader Dump
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
#endif
