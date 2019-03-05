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
#include "common.h"
#include "gui/gui.h"
#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/spi.h"

#define CS_HI() GPIO_pin_set(LCD_SPI.csn)
#define CS_LO() GPIO_pin_clear(LCD_SPI.csn)
#define CMD_MODE() GPIO_pin_clear(LCD_SPI_MODE)
#define DATA_MODE() GPIO_pin_set(LCD_SPI_MODE)

#ifndef HAS_LCD_FLIPPED
    #define HAS_LCD_FLIPPED 0
#endif

#ifndef LCD_CONTRAST_FUNC
    #define LCD_CONTRAST_FUNC(x) ((x) * 12 + 76)  // Contrsat function for devo radios
#endif
//The screen is 129 characters, but we'll only expoise 128 of them
#define PHY_LCD_WIDTH 129
#define LCD_PAGES 8
static u8 img[PHY_LCD_WIDTH * LCD_PAGES];
static u8 dirty[PHY_LCD_WIDTH];
static u16 xstart, xend;  // After introducing logical view for devo10, the coordinate can be >= 5000
static u16 xpos, ypos;
static s8 dir;

void LCD_Cmd(unsigned cmd) {
    CMD_MODE();
    CS_LO();
    spi_xfer(LCD_SPI.spi, cmd);
    CS_HI();
}

void LCD_Data(unsigned cmd) {
    DATA_MODE();
    CS_LO();
    spi_xfer(LCD_SPI.spi, cmd);
    CS_HI();
}

void lcd_display(uint8_t on)
{
    LCD_Cmd(0xAE | (on ? 1 : 0));
}

void lcd_set_page_address(uint8_t page)
{
    LCD_Cmd(0xB0 | (page & 0x07));
}

void lcd_set_column_address(uint8_t column)
{
    LCD_Cmd(0x10 | ((column >> 4) & 0x0F));  //MSB
    LCD_Cmd(column & 0x0F);                  //LSB
}

void lcd_set_start_line(int line)
{
  LCD_Cmd((line & 0x3F) | 0x40); 
}

void LCD_Contrast(unsigned contrast)
{
    //int data = 0x20 + contrast * 0xC / 10;
    LCD_Cmd(0x81);
    int c = LCD_CONTRAST_FUNC(contrast);
    LCD_Cmd(c);
}

void LCD_Init()
{
    //Initialization is mostly done in SPI Flash
    //Setup CS as B.0 Data/Control = C.5
#ifndef FLASH_SPI_CFG
    _spi_init(LCD_SPI_CFG);
#endif
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI.csn));
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI_MODE));
    GPIO_setup_output(LCD_SPI.csn, OTYPE_PUSHPULL);
    GPIO_setup_output(LCD_SPI_MODE, OTYPE_PUSHPULL);
    LCD_Cmd(0xE2);  //Reset
    volatile int i = 0x8000;
    while(i) i--;
    lcd_display(0);     // Display Off
    LCD_Cmd(0xA6);      // Normal display
    LCD_Cmd(0xA4);      // All Points Normal
    LCD_Cmd(0xA0);      // Set SEG Direction (Normal)
    if (HAS_LCD_FLIPPED) {
        LCD_Cmd(0xC8);  // Set COM Direction (Reversed)
        LCD_Cmd(0xA2);  // Set The LCD Display Driver Voltage Bias Ratio (1/9)
    } else {
        LCD_Cmd(0xEA);  // ??
        LCD_Cmd(0xC4);  // Common Output Mode Scan Rate
    }
    LCD_Cmd(0x2C);      // Power Controller:Booster ON
    i = 0x8000;
    while(i) i--;
    LCD_Cmd(0x2E);      // Power Controller: VReg ON
    i = 0x8000;
    while(i) i--;
    LCD_Cmd(0x2F);      // Power Controller: VFollower ON
    i = 0x8000;
    while(i) i--;
    lcd_set_start_line(0);
    // Display data write (6)
    // Clear the screen
    for(int page = 0; page < LCD_PAGES; page++) {
        lcd_set_page_address(page);
        lcd_set_column_address(0);
        for(int col = 0; col < PHY_LCD_WIDTH; col++)
            LCD_Data(0x00);
    }
    lcd_display(1);
    LCD_Contrast(5);
    memset(img, 0, sizeof(img));
    memset(dirty, 0, sizeof(dirty));
}

void LCD_Clear(unsigned int val)
{
    val = (val & 0xFF) ? 0xff : 0x00;
    memset(img, val, sizeof(img));
    memset(dirty, 0xFF, sizeof(dirty));
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir _dir)
{
    if (_dir == DRAW_SWNE) {
        ypos = y1;  // bug fix: must do it this way to draw bmp
        dir = -1;
    } else {
        ypos = y0;
        dir = 1;
    }
    xstart = x0;
    xend = x1;
    xpos = x0;
}
/* Screen coordinates are as follows:
 * (128, 32)   ....   (0, 32)
 *   ...       ....     ...
 * (128, 63)   ....   (0, 63)
 * (128, 0)    ....   (0, 0)
 *   ...       ....     ...
 * (128, 31)   ....   (0, 31)
 */
void LCD_DrawStop(void)
{
    int col = 0;
    int p, c;
    for (p = 0; p < LCD_PAGES; p++) {
        int init = 0;
        for (c = 0; c < PHY_LCD_WIDTH; c++) {
            if(dirty[c] & (1 << p)) {
                if(! init) {
                    lcd_set_page_address(p);
                    lcd_set_column_address(c);
                } else if(col+1 != c) {
                    lcd_set_column_address(c);
                }
                LCD_Data(img[p * PHY_LCD_WIDTH + c]);
                col = c;
            }
        }
    }
    memset(dirty, 0, sizeof(dirty));
}

void LCD_DrawPixel(unsigned int color)
{
	if (xpos < LCD_WIDTH && ypos < LCD_HEIGHT) {	// both are unsigned, can not be < 0
		int y = ypos;
		int x = xpos;
		int ycol = y / 8;
		int ybit = y & 0x07;
        if (color) {
            img[ycol * PHY_LCD_WIDTH + x] |= 1 << ybit;
        } else {
            img[ycol * PHY_LCD_WIDTH + x] &= ~(1 << ybit);
        }
        dirty[x] |= 1 << ycol;
    }
	// this must be executed to continue drawing in the next row
    xpos++;
    if (xpos > xend) {
        xpos = xstart;
        ypos += dir;
    }
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    xpos = x;
    ypos = y;
    LCD_DrawPixel(color);
}
