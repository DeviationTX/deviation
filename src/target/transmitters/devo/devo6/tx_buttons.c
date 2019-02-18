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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "common.h"
#include "config/tx.h"

#define BUTTON_LEFT_UP     (1 << BUT_TRIM_L_POS)
#define BUTTON_LEFT_DOWN   (1 << BUT_TRIM_L_NEG)
#define BUTTON_LEFT_BOTH   (1 << BUT_TRIM_L_POS | 1 << BUT_TRIM_L_NEG)
#define BUTTON_RIGHT_UP    (1 << BUT_TRIM_R_POS)
#define BUTTON_RIGHT_DOWN  (1 << BUT_TRIM_R_NEG)
#define BUTTON_RIGHT_BOTH  (1 << BUT_TRIM_R_POS | 1 << BUT_TRIM_R_NEG)
#define BUTTON_ALL         (1 << BUT_TRIM_R_POS | 1 << BUT_TRIM_R_NEG \
                          | 1 << BUT_TRIM_L_POS | 1 << BUT_TRIM_L_NEG)

static const u16 columns[] = {GPIO6, GPIO7, GPIO8, GPIO9, 0xffff};
static const u16 rows[] = {GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, 0xffff};
static const u8 buttonmap[] = {
    BUT_LEFT,        BUT_RIGHT,       BUT_ENTER,       BUT_TRIM_RH_NEG, BUT_TRIM_RH_POS,
    BUT_DOWN,        BUT_UP,          BUT_EXIT,        BUT_TRIM_LH_POS, BUT_TRIM_LH_NEG,
    BUT_LAST,        BUT_TRIM_R_NEG,  BUT_TRIM_R_POS,  BUT_TRIM_L_POS,  BUT_TRIM_L_NEG,
    BUT_TRIM_LV_NEG, BUT_TRIM_RV_POS, BUT_TRIM_LV_POS, BUT_LAST,        BUT_TRIM_RV_NEG,
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
            if((! (but & *r))
               && (!(1 << buttonmap[idx] & Transmitter.ignore_buttons))) {
                result |= 1 << (buttonmap[idx] - 1);
            }
            idx++;
        }
    }
    return result;
}

void CHAN_SetButtonCfg(const char *str)
{
    if(strcmp(str, "trim-left-both") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_LEFT_BOTH;
    } else if(strcmp(str, "trim-left-up") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_LEFT_UP;
    } else if(strcmp(str, "trim-left-down") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_LEFT_DOWN;
    } else if(strcmp(str, "trim-right-both") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_RIGHT_BOTH;
    } else if(strcmp(str, "trim-right-up") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_RIGHT_UP;
    } else if(strcmp(str, "trim-right-down") == 0) {
        Transmitter.ignore_buttons &= ~BUTTON_RIGHT_DOWN;
    } else if(strcmp(str, "trim-all") == 0) {
        Transmitter.ignore_buttons = 0;
    } else {
        Transmitter.ignore_buttons = BUTTON_ALL;
    }
}
