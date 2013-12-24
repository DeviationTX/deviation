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
#include <libopencm3/stm32/f2/gpio.h>
#include <libopencm3/stm32/f2/rcc.h>
#include "common.h"
#include "gui/gui.h"

#define LCD_CMD_ADDR ((uint32_t)FSMC_BANK1_BASE) /* Register Address */
#define LCD_DATA_ADDR ((uint32_t)FSMC_BANK1_BASE + 0x10000) /* Data Address */

#define LCD_CMD *(volatile uint8_t *)(LCD_CMD_ADDR)
#define LCD_DATA *(volatile uint8_t *)(LCD_DATA_ADDR)

//The screen is 212 characters, but we'll only expoise 128 of them
#define PHY_LCD_WIDTH 212
#define LCD_PAGES 8
static u8 img[LCD_WIDTH * LCD_PAGES];
static u8 dirty[LCD_WIDTH];
static unsigned int xstart, xend;  // After introducing logical view for devo10, the coordinate can be >= 5000
static unsigned int xpos, ypos;
static s8 dir;

#define NCS_HI()  gpio_set(  GPIOD, GPIO14)
#define NCS_LO()  gpio_clear(GPIOD, GPIO14)
#define A0_HI()   gpio_set(  GPIOD, GPIO13)
#define A0_LO()   gpio_clear(GPIOD, GPIO13)
#define RST_HI()  gpio_set(  GPIOD, GPIO12)
#define RST_LO()  gpio_clear(GPIOD, GPIO12)
#define CLK_HI()  gpio_set(  GPIOD, GPIO11)
#define CLK_LO()  gpio_clear(GPIOD, GPIO11)
#define MOSI_HI() gpio_set(  GPIOD, GPIO10)
#define MOSI_LO() gpio_clear(GPIOD, GPIO10)

#define __no_operation()  asm volatile ("nop")
void write_pixel(int bit);
void lcd_set_row(int y);

/* These are ported from th eopentx driver */
void AspiCmd(u8 Command_Byte)
{
    int i=8;
    A0_LO();
    CLK_HI();
    __no_operation();
    NCS_LO();

    while (i--) {
        CLK_LO();
        if (Command_Byte&0x80)
            MOSI_HI();
        else
            MOSI_LO();

        Command_Byte <<= 1;
        CLK_HI();
        __no_operation();
    }

    NCS_HI();  
    A0_HI();
}
void AspiData(u8 Para_data)
{
    int i=8;
    CLK_HI();
    A0_HI();
    NCS_LO();
    while (i--) {
        CLK_LO();
        if (Para_data&0x80)
            MOSI_HI();
        else
            MOSI_LO();
        Para_data <<= 1;
        __no_operation();
        CLK_HI();
        __no_operation();
    }
    NCS_HI();
    A0_HI();  
}

//275us
void lcd_delay(volatile unsigned int ms)
{
  volatile u8 i;
  while(ms != 0)
  {
    for(i=0;i<250;i++) {}
    for(i=0;i<75;i++) {}
    ms--;
  }
}
void LCD_Contrast(u8 contrast)
{
    AspiCmd(0x81);	//Set Vop
    contrast *= 25 + 5;
    AspiCmd(contrast);
}

void lcd_screen_init()
{
    AspiCmd(0x2b);   //Panel loading set ,Internal VLCD.
    AspiCmd(0x25);   //Temperature compensation curve definition: 0x25 = -0.05%/oC
    AspiCmd(0xEA);	//set bias=1/10 :Command table NO.27
    AspiCmd(0x81);	//Set Vop
    AspiCmd(0x80);      //Set contrast 0--255
    AspiCmd(0xA6);	//inverse display off
    AspiCmd(0xD1);	//SET RGB:Command table NO.21 .SET RGB or BGR.  D1=RGB
    AspiCmd(0xD5);	//set color mode 4K and 12bits  :Command table NO.22
    AspiCmd(0xA0);	//line rates,25.2 Klps
    AspiCmd(0xC8);	//SET N-LINE INVERSION
    AspiCmd(0x1D);	//Disable NIV
    AspiCmd(0xF1);	//Set CEN
    AspiCmd(0x3F);	// 1/64DUTY
    AspiCmd(0x84);	//Disable Partial Display
    AspiCmd(0xC4);	//MY=1,MX=0
    AspiCmd(0x89);	//WA=1,column (CA) increment (+1) first until CA reaches CA boundary, then RA will increment by (+1).

    AspiCmd(0xF8);	//Set Window Program Enable  ,inside modle
    AspiCmd(0xF4);   //starting column address of RAM program window.
    AspiCmd(0x00);
    AspiCmd(0xF5);   //starting row address of RAM program window.
    AspiCmd(0x60);
    AspiCmd(0xF6);   //ending column address of RAM program window.
    AspiCmd(0x47);
    AspiCmd(0xF7);   //ending row address of RAM program window.
    AspiCmd(0x9F);
}

void LCD_Init()
{
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
    //MOSI
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, GPIO10);
    //CLK
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, GPIO11);
    //RST
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, GPIO12);
    //A0
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, GPIO13);
    //CS
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, GPIO14);
    NCS_HI();

    RST_HI();
    lcd_delay(5);
    RST_LO();
    lcd_delay(120); //11ms
    RST_HI();
    lcd_delay(2500);
    AspiCmd(0xE2);
    lcd_delay(2500);

    lcd_screen_init();
    lcd_delay(120);
    lcd_screen_init();
    lcd_delay(120);
    AspiCmd(0xAF);        //dc2=1, IC into exit SLEEP MODE, dc3=1 gray=ON, dc4=1 Green Enhanc mode disabled

    memset(img, 0, sizeof(img));
    memset(dirty, 0, sizeof(dirty));

    //Clear screen
    for (int y = 0; y < LCD_HEIGHT; y++) {
        lcd_set_row(y);
        AspiCmd(0xAF);
        CLK_HI();
        A0_HI();
        NCS_LO();
        for (int x = 0; x < 212; x++) {
            //write_pixel(((x/53) % 2) ^ ((y / 16) %2));
            write_pixel(0);
        }
        NCS_HI();
        A0_HI();
        AspiData(0);
    }
}

void LCD_Clear(unsigned int val)
{
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

void lcd_set_row(int y)
{
  AspiCmd(0);	//Set Column Address LSB CA[3:0]
  AspiCmd(0x10);	//Set Column Address MSB CA[7:4]
    
  AspiCmd((y&0x0F)|0x60);	//Set Row Address LSB RA [3:0]
  AspiCmd(((y>>4)&0x0F)|0x70);    //Set Row Address MSB RA [7:4]
}
void write_pixel(int bit)
{
    for(int i = 0; i < 4; i++) {
        if (bit)
            MOSI_HI();
        else
            MOSI_LO();
        CLK_LO();
        __no_operation();
        CLK_HI();
    }
}
/* Screen is 212 x 64 with 4bpp */
void LCD_DrawStop(void)
{
    for (int y = 0; y < LCD_HEIGHT; y++) {
        if(! (dirty[y / 8] & (1 << (y % 8))))
            continue;
        u8 *p = &img[(y / 8) * LCD_WIDTH];
        u8 mask = 1 << (y % 8);

        lcd_set_row(y);
        AspiCmd(0xAF);
        CLK_HI();
        A0_HI();
        NCS_LO();
        for (int x = 0; x < LCD_WIDTH; x++) {
            write_pixel(p[x] & mask);
        }
        NCS_HI();
        A0_HI();
        AspiData(0);
    }
    memset(dirty, 0, sizeof(dirty));
}

void LCD_DrawPixel(unsigned int color)
{
    if (xpos < LCD_WIDTH && ypos < LCD_HEIGHT) {
        // both are unsigned, can not be < 0
        int y = ypos;
        int x = xpos;
        int ycol = y / 8;
        int ybit = y & 0x07;
        if(color) {
            img[ycol * LCD_WIDTH + x] |= 1 << ybit;
        } else {
            img[ycol * LCD_WIDTH + x] &= ~(1 << ybit);
        }
        dirty[ycol] |= 1 << ybit;
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
