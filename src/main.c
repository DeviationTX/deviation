/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "target.h"
#include "misc.h"
#include "gui/gui.h"

int main()
{
    PWR_Init();
    Initialize_ButtonMatrix();
    Delay(0x2710);

    LCD_Init();
    Initialize_Channels();

    SPIFlash_Init();
    CYRF_Initialize();
    UART_Initialize();
    SPITouch_Init();
    SignOn();
    LCD_Clear(0x0000);

#ifdef BL_DUMP
    LCD_PrintStringXY(40, 10, "Dumping");

    printf("Erase...\n\r");

    SPIFlash_EraseSector(0x2000);
    SPIFlash_EraseSector(0x3000);
    SPIFlash_EraseSector(0x4000);
    SPIFlash_EraseSector(0x5000);

    printf("Pgm 2\n\r");
    SPIFlash_WriteBytes(0x2000, 0x1000, (u8*)0x08000000);
    printf("Pgm 3\n\r");
    SPIFlash_WriteBytes(0x3000, 0x1000, (u8*)0x08001000);
    printf("Pgm 4\n\r");
    SPIFlash_WriteBytes(0x4000, 0x1000, (u8*)0x08002000);
    printf("Pgm 5\n\r");
    SPIFlash_WriteBytes(0x5000, 0x1000, (u8*)0x08003000);
    printf("Done\n\r");

    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Done");

    while(1) 
    {
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
#endif 

#ifdef STATUS_SCREEN
    u32 last_buttons = 0;
    char str[80];
#ifdef HAS_FS
    void PushMeButton(void) {
    	LCD_PrintStringXY(100, 130, "You pushed it.");
    }
    LCD_DrawWindowedImageFromFile(0, 0, "devo8.bmp", 0, 0, 0, 0);
    /* SRC image is 90x24, when called below with 90x24 it gives out of bounds return on image draw
     * however if I give it 1 pixel less on height and width it draws but wraps... */
    int testButton = GUI_CreateButton(100,150,89,23,"Push Me",PushMeButton);
#else
    LCD_DrawCircle(200, 200, 40, 0xF800);
#endif
    {
        u8 * pBLString = BOOTLOADER_Read(BL_ID);
        (u8*)0x08001000;
        u8 mfgdata[6];
        sprintf(str, "BootLoader   : %s\n",pBLString);
        LCD_PrintStringXY(10, 10, str);
        sprintf(str, "SPI Flash    : %X\n",(unsigned int)SPIFlash_ReadID());
        LCD_PrintString(str);
        CYRF_GetMfgData(mfgdata);
        sprintf(str, "CYRF Mfg Data: %02X%02X%02X%02X%02X%02X\n",
            mfgdata[0],
            mfgdata[1],
            mfgdata[2],
            mfgdata[3],
            mfgdata[4],
            mfgdata[5]);
        LCD_PrintString(str);
    }
    while(1) {
        int i;
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
        u32 buttons = ScanButtons();

        /* GUI Handling
         * We beed to handle screen redraws here
         * */

        if(buttons != last_buttons) {
            last_buttons = buttons;
            LCD_PrintStringXY(10, 50, "Buttons:\n");
            for(i = 0; i < 32; i++)
                LCD_PrintChar((buttons & (1 << i)) ? '0' : '1');
        }
        u16 voltage = PWR_ReadVoltage();
        sprintf(str, "Voltage: %2d.%03d\n", voltage >> 12, voltage & 0x0fff);
        LCD_PrintStringXY(10, 70, str);
        u16 throttle = ReadThrottle();
        u16 rudder = ReadRudder();
        u16 aileron = ReadAileron();
        u16 elevator = ReadElevator();
        sprintf(str, "Throttle: %04X Elevator: %04X\n", throttle, elevator);
        LCD_PrintString(str);
        sprintf(str, "Rudder  : %04X Aileron : %04X\n", rudder, aileron);
        LCD_PrintString(str);
        if(SPITouch_IRQ()) {
            struct touch t = SPITouch_GetCoords();

            sprintf(str, "x : %4d y : %4d\n", t.x, t.y);
            LCD_PrintString(str);
            sprintf(str, "z1: %4d z2: %4d\n", t.z1, t.z2);
            LCD_PrintString(str);
            GUI_CheckTouch(t);
        }
    }
#endif    
#ifdef SCANNER
    #define NUM_CHANNELS    0x50

    u32 i,j,k;
    u8 dpbuffer[16];
    u8 channelnoise[NUM_CHANNELS];
    u8 channel = 0x04;

    CYRF_ConfigRxTx(1);
    CYRF_ConfigCRCSeed(0);
    CYRF_ConfigSOPCode(0);

    while(1)
    {
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();

        CYRF_ConfigRFChannel(channel);
        CYRF_StartReceive();
        Delay(10);

        CYRF_ReadDataPacket(dpbuffer);
        channelnoise[channel] = CYRF_ReadRSSI(1);

        printf("%02X : %d\n\r",channel,channelnoise[channel]);
    
        channel++;
        if(channel == NUM_CHANNELS)
        {
            channel = 0x04;
            LCD_Clear(0x0000);

            for(i=4;i<NUM_CHANNELS;i++)
            {
                LCD_SetDrawArea(30 + (3*i), 30, 31 + (3*i), 190);
                LCD_DrawStart();        
                for(k=0;k<16; k++)
                {
                    for(j=0; j<2; j++)
                    {
                        if(k < (15 - channelnoise[i]))
                        {
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                            LCD_DrawPixel(0xF000);
                        }
                        else
                        {
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                            LCD_DrawPixel(0xFFFF);
                        }
                    }
                }
                LCD_DrawStop();
            }
        }
    }

#endif
    return 0;
}

