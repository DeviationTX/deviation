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
#include "target.h"

static const u16 columns[] = {GPIO6, GPIO7, GPIO8, GPIO9, 0xffff};
static const u16 rows[] = {GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, 0xffff};
static const u8 buttonmap[] = {
    BUT_LEFT,        BUT_RIGHT,       BUT_ENTER,       BUT_AILTRIM_POS, BUT_AILTRIM_NEG,
    BUT_DOWN,        BUT_UP,          BUT_EXIT,        BUT_RUDTRIM_NEG, BUT_RUDTRIM_POS,
    BUT_NOCON_1,     BUT_RGTTRIM_NEG, BUT_RGTTRIM_POS, BUT_LFTTRIM_POS, BUT_LFTTRIM_NEG,
    BUT_ELETRIM_NEG, BUT_THRTRIM_POS, BUT_ELETRIM_POS, BUT_NOCON_2,     BUT_THRTRIM_NEG,
    };
    
const char *tx_button_str[] = {
    "AIL-",
    "AIL+",
    "ELE-",
    "ELE+",
    "THR-",
    "THR+",
    "RUD-",
    "RUD+",
    "LEFT-",
    "LEFT+",
    "RIGHT-",
    "RIGHT+",
    "Left",
    "Right",
    "Down",
    "Up",
    "Enter"
    "Exit",
};
#define COL_PORT GPIOB
#define COL_PORT_MASK (GPIO6 | GPIO7 | GPIO8 | GPIO9)
#define ROW_PORT GPIOE


void Initialize_ButtonMatrix()
{
  /* Enable AFIO */
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN);

  /* Remap GPIO_Remap_SWJ_JTAGDisable */
  AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

  /* Enable GPIOB & GPIOE */
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPEEN);
  
  /* PortB 6-9 are open-drain output */
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                GPIO_CNF_OUTPUT_PUSHPULL, GPIO6 | GPIO7 | GPIO8 | GPIO9);

  /* PB.6 = 1, PB.7 = 0, PB.8 = 1, PB.9 = 1 */
  gpio_set(GPIOB, GPIO6 | GPIO8 | GPIO9);
  gpio_clear(GPIOB, GPIO7);

  /* PortE 2-6 are pull-up inputs */
  gpio_set_mode(GPIOE, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_PULL_UPDOWN, GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6);
  gpio_set(GPIOE, GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6);
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
