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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include "common.h"

static const u16 columns[] = {GPIO6, GPIO7, GPIO8, GPIO9, 0xffff};
static const u16 rows[] = {GPIO5, GPIO6, GPIO7, GPIO8, GPIO9, 0xffff};
static const u8 buttonmap[] = {
    BUT_LAST,        BUT_UP,          BUT_DOWN,        BUT_EXIT,        BUT_TRIM_LH_POS,
    BUT_RIGHT,       BUT_LEFT,        BUT_ENTER,       BUT_LAST,        BUT_TRIM_LH_NEG,
    BUT_TRIM_L_POS,  BUT_TRIM_L_NEG,  BUT_TRIM_R_POS,  BUT_TRIM_R_NEG,  BUT_TRIM_LV_NEG,
    BUT_TRIM_RV_POS, BUT_TRIM_RH_POS, BUT_TRIM_RH_NEG, BUT_TRIM_RV_NEG, BUT_TRIM_LV_POS,
    };
    
#define COL_PORT GPIOC
#define COL_PORT_MASK (GPIO6 | GPIO7 | GPIO8 | GPIO9)
#define ROW_PORT GPIOB
#define ROW_PORT_MASK (GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9)


void Initialize_ButtonMatrix()
{
  /* Enable AFIO */
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN);

  /* Remap GPIO_Remap_SWJ_JTAGDisable */
  AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

  /* Enable GPIOB & GPIOE */
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
  
  gpio_set_mode(COL_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, COL_PORT_MASK);
  gpio_set(COL_PORT, COL_PORT_MASK);

  gpio_set_mode(ROW_PORT, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_PULL_UPDOWN, ROW_PORT_MASK);
  gpio_set(ROW_PORT, ROW_PORT_MASK);
}
u32 ScanButtons()
{
    u8 idx = 0;
    u32 result = 0;
    const u16 *c, *r;
    gpio_set(COL_PORT, COL_PORT_MASK);
    for(c = columns; *c != 0xffff; c++) {
        gpio_clear(COL_PORT, *c);
        u16 but = gpio_port_read(ROW_PORT);
        gpio_set(COL_PORT, *c);
        for(r = rows; *r != 0xffff; r++) {
            if(! (but & *r)) {
                result |= 1 << (buttonmap[idx] - 1);
            }
            idx++;
        }
    }
    return result;
}
