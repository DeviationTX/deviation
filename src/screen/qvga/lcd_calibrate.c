/***********************************
This is a our graphics core library, for all our displays. 
We'll be adapting all the
existing libaries to use this core to make updating, support 
and upgrading easier!

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/


#include "target.h"
#include <stdlib.h>
#include <stdio.h>

void get_coords(struct touch *t)
{
    /* Wait for button press */
    while(! SPITouch_IRQ())
        ;
    *t = SPITouch_GetCoords();
    /* Wait for button releasde */
    while(SPITouch_IRQ())
        ;
}

void draw_target(u16 x, u16 y)
{
    LCD_DrawFastHLine(x - 5, y, 11, 0xffff);
    LCD_DrawFastVLine(x, y - 5, 11, 0xffff);
}

void LCD_CalibrateTouch(void)
{
    struct touch t1, t2;
    u32 xscale, yscale;
    s32 xoff, yoff;
    /* Reset calibration */
    SPITouch_Calibrate(0x10000, 0x10000, 0, 0);
    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Touch target 1");
    draw_target(20, 20);
    get_coords(&t1);

    LCD_Clear(0x0000);
    LCD_PrintStringXY(40, 10, "Touch target 2");
    draw_target(300, 220);
    get_coords(&t2);

    xscale = 280 * 0x10000 / abs(t1.x - t2.x);
    yscale = 200 * 0x10000 / abs(t1.y - t2.y);
    xoff = 20 - t1.x * xscale / 0x10000;
    yoff = 20 - t1.y * yscale / 0x10000;
    printf("Debug: scale(%u, %u) offset(%d, %d)\n", (unsigned int)xscale, (unsigned int)yscale, (int)xoff, (int)yoff);
    SPITouch_Calibrate(xscale, yscale, xoff, yoff);
}
    
