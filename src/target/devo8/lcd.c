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
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include <libopencm3/stm32/timer.h>
#include "target.h"
#include "misc.h"

#define LCD_REG_ADDR  ((uint32_t)FSMC_BANK1_BASE)    /* Register Address */
#define LCD_DATA_ADDR  ((uint32_t)FSMC_BANK1_BASE + 0x20000)  /* Data Address */

#define LCD_REG  *(volatile uint16_t *)(LCD_REG_ADDR)
#define LCD_DATA *(volatile uint16_t *)(LCD_DATA_ADDR)

void lcd_cmd(uint8_t addr, uint8_t data)
{
    LCD_REG = addr;
    LCD_DATA = data;
}
void LCD_DrawPixel(unsigned int color)
{
    LCD_DATA = color;
}
void LCD_DrawStart(void)
{
    LCD_REG = 0x22;
}
void LCD_DrawStop(void)
{
  return;
}

void LCD_SetDrawArea(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1)
{
  lcd_cmd(0x03, (x0>>0)); //set x0
  lcd_cmd(0x02, (x0>>8)); //set x0
  lcd_cmd(0x05, (x1>>0)); //set x1
  lcd_cmd(0x04, (x1>>8)); //set x1
  lcd_cmd(0x07, (y0>>0)); //set y0
  lcd_cmd(0x06, (y0>>8)); //set y0
  lcd_cmd(0x09, (y1>>0)); //set y1
  lcd_cmd(0x08, (y1>>8)); //set y1

  return;
}

void lcd_init_backlight()
{

    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO1);

    rcc_peripheral_enable_clock(&RCC_APB1ENR, RCC_APB1ENR_TIM3EN);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT,
                    TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM3, 0);
    timer_set_repetition_counter(TIM3, 0);
    timer_set_period(TIM3, 0x2CF);


    timer_set_oc_mode(TIM3, TIM_OC4, TIM_OCM_PWM1);
    timer_set_oc_value(TIM3, TIM_OC4, 0x168);
    timer_enable_oc_output(TIM3, TIM_OC4);
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

  //driving ability
  lcd_cmd(0xEA, 0x00);
  lcd_cmd(0xEB, 0x20);
  lcd_cmd(0xEC, 0x0C);
  lcd_cmd(0xED, 0xC4);
  lcd_cmd(0xE8, 0x40);
  lcd_cmd(0xE9, 0x38);
  lcd_cmd(0xF1, 0x01);
  lcd_cmd(0xF2, 0x10);
  lcd_cmd(0x27, 0xA3);

  //power voltage
  lcd_cmd(0x1B, 0x1B);
  lcd_cmd(0x1A, 0x01);
  lcd_cmd(0x24, 0x2F);
  lcd_cmd(0x25, 0x57);

  //VCOM offset
  lcd_cmd(0x23, 0x8D); //for flicker adjust

//ownTry
//lcd_cmd(0x29,0x06);
//lcd_cmd(0x2A,0x00);
//lcd_cmd(0x2C,0x06);
//lcd_cmd(0x2D,0x06);

  //power on
  lcd_cmd(0x18, 0x36);
  lcd_cmd(0x19, 0x01); //start osc
  lcd_cmd(0x01, 0x00); //wakeup
  lcd_cmd(0x1F, 0x88);
  Delay(5);
  lcd_cmd(0x1F, 0x80);
  Delay(5);
  lcd_cmd(0x1F, 0x90);
  Delay(5);
  lcd_cmd(0x1F, 0xD0);
  Delay(5);

  lcd_cmd(0x16, 0x68); //MY=0 MX=1 MV=1 ML=0 BGR=1

  //color selection
  lcd_cmd(0x17, 0x05); //0x0005=65k, 0x0006=262k

  //panel characteristic
  lcd_cmd(0x36, 0x00);

  //display on
  lcd_cmd(0x28, 0x38);
  Delay(40);
  lcd_cmd(0x28, 0x3C);

  lcd_init_backlight();
}

