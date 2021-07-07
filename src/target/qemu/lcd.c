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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include "common.h"
#include "lcd.h"

u8 screen_flip;
const struct lcdtype *disp_type;

void lcd_cmd(uint8_t addr, uint8_t data)
{
}

void lcd_set_pos(unsigned int x0, unsigned int y0)
{
}

void LCD_DrawPixel(unsigned int color)
{
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
}

void LCD_DrawStop(void)
{
}

void LCD_Sleep()
{
}

void LCD_Init()
{
}

