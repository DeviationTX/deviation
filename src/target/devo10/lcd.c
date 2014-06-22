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
#include "gui/gui.h"

#define LCD_CMD_ADDR ((uint32_t)FSMC_BANK1_BASE) /* Register Address */
#define LCD_DATA_ADDR ((uint32_t)FSMC_BANK1_BASE + 0x10000) /* Data Address */

#define LCD_CMD *(volatile uint8_t *)(LCD_CMD_ADDR)
#define LCD_DATA *(volatile uint8_t *)(LCD_DATA_ADDR)

//The screen is 129 characters, but we'll only expoise 128 of them
#define PHY_LCD_WIDTH 129
#define LCD_PAGES 8
static u8 img[PHY_LCD_WIDTH * LCD_PAGES];
static u8 dirty[PHY_LCD_WIDTH];
static unsigned int xstart, xend;  // After introducing logical view for devo10, the coordinate can be >= 5000
static unsigned int xpos, ypos;
static s8 dir;

void lcd_display(uint8_t on)
{
    LCD_CMD = 0xAE | (on ? 1 : 0);
}

void lcd_write_display_data(uint8_t display_data)
{
    LCD_DATA = display_data;
}

void lcd_set_page_address(uint8_t page)
{
    LCD_CMD = 0xB0 | (page & 0x07);
}

void lcd_set_column_address(uint8_t column)
{
    LCD_CMD = 0x10 | ((column >> 4) & 0x0F);  //MSB
    LCD_CMD = column & 0x0F;                  //LSB
}

void lcd_set_start_line(int line)
{
  LCD_CMD = (line & 0x3F) | 0x40; 
}

void LCD_Contrast(unsigned contrast)
{
    int data = 0x20 + contrast * 0xC / 10;
    LCD_CMD = 0x81;
    LCD_CMD = data & 0x3F;
}

void LCD_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPDEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPEEN);
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_FSMCEN);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO0 | GPIO1 | GPIO10 | GPIO14 | GPIO15); //D2|D3|D0|D1

    gpio_set_mode(GPIOE, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO7 | GPIO8 | GPIO9 | GPIO10); // D4|D5|D6|D7

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ, // A16
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO11);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ, // NOE|NWE
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO4 | GPIO5);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ, // NE1 (not connectred)
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO7);

    /* Normal mode, write enable, 8 bit access, SRAM, bank enabled */
    FSMC_BCR1 = (FSMC_BCR1 & 0x000000C0) | FSMC_BCR_WREN | FSMC_BCR_MBKEN;

    /* Read & write timings */
    /* Data Setup > 90ns, Address Setup = 2xHCLK to ensure no output collision in 6800
       mode since LCD E and !CS always active */
    FSMC_BTR1 = FSMC_BTR_DATASTx(7) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(2);

    // LCD bias setting (11); 0xA2; 1/9
    LCD_CMD = 0xA2;

    // ADC selection (8) -> ADdressCounter; 0xA0; inc
    LCD_CMD = 0xA0;

    //Common output mode selection (15); 0xC0; normal scan
    LCD_CMD = 0xC0;
    Delay(5);

    //Setting built-in resistance ratio (17); 0x24; default=7.50
    LCD_CMD = 0x24;

    //Electronic volume control (18) -> LCD brightness; 0x20; default=32d
    LCD_CMD = 0x81;
    LCD_CMD = 0x25 & 0x3F; // = LCD_Contrast(5);
    Delay(5);

    //Power control setting (16); V/B, V/R, V/F are used
    LCD_CMD = 0x2F;
    Delay(5);

    // Read-Modify-Write (12); let read data does not increment column address
    LCD_CMD = 0xE0;

    lcd_set_start_line(0);
    lcd_set_page_address(0);
    lcd_set_column_address(0);
    // Display data write (6)
    lcd_display(1);
    memset(img, 0, sizeof(img));
    memset(dirty, 0, sizeof(dirty));
}

void LCD_Clear(unsigned int val)
{
    /* int i,j;
    val = (val & 0xFF);
    for (i=0; i<LCD_PAGES; i++) {
        lcd_set_page_address(i);
        lcd_set_column_address(0);
        for (j=0;j<PHY_LCD_WIDTH;j++)
            LCD_DATA = val;
    } */
	// Bug fix: above method doesn't work properly, especially during keyboard type changing.
	LCD_FillRect(0, 0, LCD_WIDTH, LCD_HEIGHT, val);
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
                LCD_DATA = img[p * PHY_LCD_WIDTH + c];
                col = c;
            }
        }
    }
    memset(dirty, 0, sizeof(dirty));
}

/*
 * 2.
Display Start Line Set
Specifies line address (refer to Figure 6) to determine the initial display line, or COM0. The RAM
display data becomes the top line of LCD screen. The higher number of
lines in ascending order,
corresponding to the duty cycle follows it. When this command changes the line address, smooth
scrolling or a page change takes place.
A0 (E /RD) (R/W /WR) D7 D6 D5 D4 D3 D2 D1 D0 Hex
0     1        0      0  1 A5 A4 A3 A2 A1 A0 40h to 7Fh

8.
ADC Select
Changes the relationship between RAM column address and segment driver. The order of
segment driver output pads could be reversed by software. This allows flexi
ble IC layout during
LCD module assembly. For details, refer to the column address section of Figure 4. When display
data is written or read, the column address is incremented by 1 as shown in Figure 4.
A0 (E /RD) (R/W /WR) D7 D6 D5 D4 D3 D2 D1 D0 Hex Setting
 0    1        0      1  0  1  0  0  0  0  0 A0h Normal
                                           1 A1h Reverse
 */


void LCD_DrawPixel(unsigned int color)
{
	if (xpos < LCD_WIDTH && ypos < LCD_HEIGHT) {	// both are unsigned, can not be < 0
		int y;
		int x = PHY_LCD_WIDTH - 1 - xpos; //We want to map 0 -> 128 and 128 -> 0

		if (ypos > 31)
			y = ypos - 32;
		else
			y = ypos + 32;


		int ycol = y / 8;
		int ybit = y & 0x07;
		if(color) {
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
