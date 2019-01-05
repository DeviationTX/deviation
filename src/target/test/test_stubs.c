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
#include "common.h"
#include "mixer.h"
#include "config/tx.h"

#ifdef LCD_EMU_LOWLEVEL
#define LCD_Init EMULCD_Init
#endif

void start_event_loop()
{
}

void set_stick_positions()
{
}

void LCD_Init()
{
}

struct touch SPITouch_GetCoords()
{
    struct touch t = {256, 16, 0, 0};
    return t;
}

int SPITouch_IRQ()
{
    return 0;
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)dir;
}

void LCD_DrawStop(void) {
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    (void)x;
    (void)y;
    (void)color;
}

u32 ScanButtons()
{
    return 1;
}

int PWR_CheckPowerSwitch()
{
    return 0;
}

void PWR_Shutdown()
{
}

u32 ReadFlashID()
{
    return 0;
}

void SPITouch_Calibrate(s32 xscale, s32 yscale, s32 xoff, s32 yoff)
{
    (void)xscale;
    (void)yscale;
    (void)xoff;
    (void)yoff;
}

void CLOCK_Init()
{
}
void CLOCK_StartTimer(unsigned us, u16 (*cb)(void))
{
    (void)us;
    (void)cb;
}

void CLOCK_StopTimer()
{
}

void CLOCK_SetMsecCallback(int cb, u32 msec)
{
    (void)msec;
    (void)cb;
}

void CLOCK_ClearMsecCallback(int cb)
{
    (void)cb;
}

u32 CLOCK_getms()
{
    return 100000;
}

void PWR_Sleep()
{
}

void LCD_ForceUpdate()
{
}
