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

int main()
{

    Initialize_PowerSwitch();
    Initialize_ButtonMatrix();
    Delay(0x2710);

    Initialize_Clock();

    LCD_Init();
    Initialize_Channels();

    Initialize_SPIFlash();
    Initialize_SPICYRF();
    Initialize_UART();
    SignOn();
	
#ifdef BL_DUMP
    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Dumping");

    printf("Erase...\n\r");

    EraseSector(0x2000);
    EraseSector(0x3000);
    EraseSector(0x4000);
    EraseSector(0x5000);

    printf("Pgm 2\n\r");
    WriteBytes(0x2000, 0x1000, (u8*)0x08000000);
    printf("Pgm 3\n\r");
    WriteBytes(0x3000, 0x1000, (u8*)0x08001000);
    printf("Pgm 4\n\r");
    WriteBytes(0x4000, 0x1000, (u8*)0x08002000);
    printf("Pgm 5\n\r");
    WriteBytes(0x5000, 0x1000, (u8*)0x08003000);
    printf("Done\n\r");

    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Done");

    while(1) 
    {
        if(CheckPowerSwitch())
            PowerDown();
    }
#endif 

#ifdef HELLO_WORLD
    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Hello\n");
    LCD_PrintString("World");

    while(1) {
        int i;
        if(CheckPowerSwitch())
            PowerDown();
        u32 buttons = ScanButtons();
        LCD_SetXY(0, 60);
        for(i = 0; i < 32; i++)
            LCD_PrintChar((buttons & (1 << i)) ? '0' : '1');
        u16 throttle = ReadThrottle();
        LCD_PrintChar('\n');
        for(i = 11; i >= 0; i--)
            LCD_PrintChar((throttle & (1 << i)) ? '0' : '1');
    }
#endif    
#ifdef SCANNER
    #define NUM_CHANNELS    0x50

    u32 i,j,k;
    u8 dpbuffer[16];
    u8 channelnoise[NUM_CHANNELS];
    u8 channel = 0x04;

    ConfigRxTx(1);
    ConfigCRCSeed(0);
    ConfigSOPCode(0);

    while(1)
    {
        if(CheckPowerSwitch())
            PowerDown();

        ConfigRFChannel(channel);
        StartReceive();
        Delay(10);

        ReadDataPacket(dpbuffer);
        channelnoise[channel] = ReadRSSI(1);

        printf("%02X : %d\n\r",channel,channelnoise[channel]);
    
        channel++;
        if(channel == NUM_CHANNELS)
        {
            channel = 0x04;
            LCD_Clear(0x0000);

            for(i=4;i<NUM_CHANNELS;i++)
            {
                lcd_area(30 + (3*i), 30, 31 + (3*i), 190);
                lcd_drawstart();        
                for(k=0;k<16; k++)
                {
                    for(j=0; j<2; j++)
                    {
                        if(k < (15 - channelnoise[i]))
                        {
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                            lcd_draw(0xF000);
                        }
                        else
                        {
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                            lcd_draw(0xFFFF);
                        }
                    }
                }
                lcd_drawstop();
            }
        }
    }

#endif
    return 0;
}

