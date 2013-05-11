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
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include "common.h"
#include "lcd.h"

u8 screen_flip;
const struct lcdtype *disp_type;

void lcd_cmd(uint8_t addr, uint8_t data)
{
    LCD_REG = addr;
    LCD_DATA = data;
}

void lcd_set_pos(unsigned int x0, unsigned int y0)
{
    if (screen_flip)
        y0 = 239 - y0;
    disp_type->set_pos(x0, y0);
    //printf("lcd_set_pos: %d, %d\n", x0, y0);
}

#define LCDTYPE_HX8347  1
#define LCDTYPE_ILI9341 2
#define LCDTYPE_UNKNOWN 0
int lcd_detect()
{
    LCD_REG = 0x00;
    u8 data = LCD_DATA;
    if (data == 0x47) {
        return LCDTYPE_HX8347;
    } else {
        LCD_REG = 0xd3;
        data = LCD_DATA;
        data = LCD_DATA;
        data = LCD_DATA;
        u16 data2 = LCD_DATA;
        data2 = (((int)data) << 8) | data2;
        if (data2 != 0x9341) {
            return LCDTYPE_UNKNOWN;
        } else {
            return LCDTYPE_ILI9341;
        }
    }
}

void LCD_DrawPixel(unsigned int color)
{
    LCD_DATA = color;
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    lcd_set_pos(x, y);
    LCD_DATA = color;
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
  if (dir == DRAW_SWNE) {
    unsigned int y = 239 - y0;
    y0 = 239 - y1;
    y1 = y;
    screen_flip = 1;
  } else {
    screen_flip = 0;
  }
  //printf("LCD_DrawStart: (%d, %d) - (%d, %d)\n", x0, y0, x1, y1);
  disp_type->draw_start(x0, y0, x1, y1);
  return;
}

void LCD_DrawStop(void)
{
  return;
}

void LCD_Sleep()
{
    disp_type->sleep();
}

void LCD_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPDEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPEEN);
    rcc_peripheral_enable_clock(&RCC_AHBENR, RCC_AHBENR_FSMCEN);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO0 | GPIO1 | GPIO8 | GPIO9 | GPIO10 | GPIO14 | GPIO15);

    gpio_set_mode(GPIOE, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO7 | GPIO8 | GPIO9 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO11);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO4 | GPIO5);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  GPIO7);

    /* Extended mode, write enable, 16 bit access, bank enabled */
    FSMC_BCR1 = FSMC_BCR_MWID | FSMC_BCR_WREN | FSMC_BCR_MBKEN;

    /* Read & write timings */
    FSMC_BTR1  = FSMC_BTR_DATASTx(2) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(1) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B);
    FSMC_BWTR1 = FSMC_BTR_DATASTx(2) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(1) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B);
    int type = lcd_detect();
    if (type == LCDTYPE_ILI9341) {
        ili9341_init();
    } else if (type == LCDTYPE_HX8347) {
        hx8347_init();
    } else {
        printf("No LCD detected\n");
    }
}

