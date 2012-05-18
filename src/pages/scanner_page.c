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
#include "pages.h"

void PAGE_ScannerInit(int page)
{
    (void)page;
    CYRF_ConfigRxTx(1);
    CYRF_ConfigCRCSeed(0);
    CYRF_ConfigSOPCode(0);
}

void PAGE_ScannerEvent()
{
#define NUM_RADIOCHANNELS    0x50

    u32 i,j,k;
    u8 dpbuffer[16];
    static u8 channelnoise[NUM_RADIOCHANNELS];
    static u8 channel = 0x04;


    CYRF_ConfigRFChannel(channel);
    CYRF_StartReceive();
    Delay(10);

    CYRF_ReadDataPacket(dpbuffer);
    channelnoise[channel] = CYRF_ReadRSSI(1);

    printf("%02X : %d\n",channel,channelnoise[channel]);

    channel++;
    if(channel == NUM_RADIOCHANNELS)
    {
        channel = 0x04;
        LCD_Clear(0x0000);

        for(i=4;i<NUM_RADIOCHANNELS;i++)
        {
            LCD_DrawStart(30 + (3*i), 30, 31 + (3*i), 190, DRAW_NWSE);
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

int PAGE_ScannerCanChange()
{
    return 1;
}
