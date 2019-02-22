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
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "gui/gui.h"
#include "target/drivers/mcu/stm32/rcc.h"

#define CS_HI() GPIO_pin_set(LCD_SPI.csn)
#define CS_LO() GPIO_pin_clear(LCD_SPI.csn)
#define CMD_MODE() GPIO_pin_clear(LCD_SPI_MODE)
#define DATA_MODE() GPIO_pin_set(LCD_SPI_MODE)

#define PHY_LCD_WIDTH 128
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
    LCD_Cmd(0x81);
    int c = contrast * contrast * 255 / 100; //contrast should range from 0 to 255
    LCD_Cmd(c);
}

void LCD_Init()
{
    //Initialization is mostly done in SPI Flash
    //Setup CS as B.0 Data/Control = C.5
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI.csn));
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI_MODE));
    GPIO_setup_output(LCD_SPI.csn, OTYPE_PUSHPULL);
    GPIO_setup_output(LCD_SPI_MODE, OTYPE_PUSHPULL);
    _msleep(100);
    lcd_display(0); //Display Off
    LCD_Cmd(0xD5);  //Set Display Clock Divide Ratio / OSC Frequency
    LCD_Cmd(0x80);  //Display Clock Divide Ratio / OSC Frequency
    LCD_Cmd(0xA8);  //Set Multiplex Ratio
    LCD_Cmd(0x3F);  //Multiplex Ratio for 128x64 (LCD_HEIGHT - 1)
    LCD_Cmd(0xD3);  //Set Display Offset
    LCD_Cmd(0x00);  //Display Offset (0)
    LCD_Cmd(0x40);  //Set Display Start Line (0)
    LCD_Cmd(0x8D);  //Set Charge Pump
    LCD_Cmd(0x10);  //Charge Pump (0x10 External, 0x14 Internal DC/DC)
    LCD_Cmd(0xA1);  //Set Segment Re-Map (Reversed)
    LCD_Cmd(0xC8);  //Set Com Output Scan Direction (Reversed)
    LCD_Cmd(0xDA);  //Set COM Hardware Configuration
    LCD_Cmd(0x12);  //COM Hardware Configuration
    LCD_Cmd(0xD9);  //Set Pre-Charge Period
    LCD_Cmd(0x4F);  //Set Pre-Charge Period (A[7:4]:Phase 2, A[3:0]:Phase 1)
    LCD_Cmd(0xDB);  //Set VCOMH Deselect Level
    LCD_Cmd(0x20);  //VCOMH Deselect Level (0x00 ~ 0.65 x VCC, 0x10 ~ 0.71 x VCC, 0x20 ~ 0.77 x VCC, 0x30 ~ 0.83 x VCC)
    LCD_Cmd(0xA4);  //Disable Entire Display On
    LCD_Cmd(0xA6);  //Set Normal Display (not inverted)
    LCD_Cmd(0x2E);  //Deactivate scroll
    //Clear the screen
    for(int page = 0; page < LCD_PAGES; page++) {
        lcd_set_page_address(page);
        lcd_set_column_address(0);
        for(int col = 0; col < PHY_LCD_WIDTH; col++)
            LCD_Data(0x00);
    }
    lcd_display(1); //Display On
    LCD_Contrast(5);
    memset(img, 0, sizeof(img));
    memset(dirty, 0, sizeof(dirty));
    _msleep(100);
}

void BACKLIGHT_Init()
{
    rcc_periph_clock_enable(get_rcc_from_pin(BACKLIGHT_TIM.pin));
    //Turn off backlight
    GPIO_setup_input(BACKLIGHT_TIM.pin, ITYPE_FLOAT);
}

void BACKLIGHT_Brightness(unsigned brightness)
{
    LCD_Contrast(brightness);
    if (brightness == 0) {
        lcd_display(0); //Display Off
        //Charge Pump disable
        //LCD_Cmd(0x8D);  //Set Charge Pump
        //LCD_Cmd(0x10);  //Charge Pump (0x10 External, 0x14 Internal DC/DC)        
    } else {
        //Charge Pump enable
        //LCD_Cmd(0x8D);  //Set Charge Pump
        //LCD_Cmd(0x14);  //Charge Pump (0x10 External, 0x14 Internal DC/DC)
        lcd_display(1); //Display On
    }
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
