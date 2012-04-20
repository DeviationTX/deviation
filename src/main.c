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

void Delay(u32 count)
{
    while(count) {
        int i = 72000;
        while(i)
            i--;
        count--;
    }
}

void Hang()
{
    while(1)
        ;
}

int main()
{

    Initialize_PowerSwitch();
    Initialize_ButtonMatrix();
    Delay(0x2710);

    Initialize_Clock();

    Initialize_LCD();
    Initialize_Backlight();
    Initialize_Channels();

    Initialize_SPIFlash();
    Initialize_SPICYRF();
    Initialize_UART();
    SignOn();
	
#ifdef BL_DUMP
    lcd_clear(0x0000);
    lcd_writeCharacter('D', 40,  10);    
    lcd_writeCharacter('U', 50,  10);    
    lcd_writeCharacter('M', 60,  10);    
    lcd_writeCharacter('P', 70,  10);    
    lcd_writeCharacter('I', 80,  10);    
    lcd_writeCharacter('N', 90,  10);    
    lcd_writeCharacter('G', 100, 10);    

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

    lcd_clear(0x0000);
    lcd_writeCharacter('D', 40,  10);    
    lcd_writeCharacter('O', 50,  10);    
    lcd_writeCharacter('N', 60,  10);    
    lcd_writeCharacter('E', 70,  10);    

    while(1) 
    {
        if(CheckPowerSwitch())
            PowerDown();
    }
#endif 

#ifdef HELLO_WORLD
    lcd_clear(0x0000);
    lcd_writeCharacter('H', 40, 10);    
    lcd_writeCharacter('E', 50, 10);    
    lcd_writeCharacter('L', 60, 10);    
    lcd_writeCharacter('L', 70, 10);    
    lcd_writeCharacter('O', 80, 10);    
    lcd_writeCharacter('W', 40, 30);    
    lcd_writeCharacter('O', 50, 30);    
    lcd_writeCharacter('R', 60, 30);    
    lcd_writeCharacter('L', 70, 30);    
    lcd_writeCharacter('D', 80, 30);    

    while(1) {
        int i;
        if(CheckPowerSwitch())
            PowerDown();
        u32 buttons = ScanButtons();
        for(i = 0; i < 32; i++)
            lcd_writeCharacter((buttons & (1 << i)) ? '0' : '1', i << 3, 60);
        u16 throttle = ReadThrottle();
        for(i = 11; i >= 0; i--)
            lcd_writeCharacter((throttle & (1 << i)) ? '0' : '1', i << 3, 80);
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
            lcd_clear(0x0000);

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

