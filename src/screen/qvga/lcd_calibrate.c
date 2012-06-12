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
    s32 xscale, yscale;
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

    printf("T1:(%d, %d)\n", t1.x, t1.y);
    printf("T2:(%d, %d)\n", t2.x, t2.y);
    xscale = t2.x - t1.x;
    xscale = 280 * 0x10000 / xscale;
    yscale = t2.y - t1.y;
    yscale = 200 * 0x10000 / yscale;
    xoff = 20 - t1.x * xscale / 0x10000;
    yoff = 20 - t1.y * yscale / 0x10000;
    printf("Debug: scale(%d, %d) offset(%d, %d)\n", (int)xscale, (int)yscale, (int)xoff, (int)yoff);
    SPITouch_Calibrate(xscale, yscale, xoff, yoff);
}
    
