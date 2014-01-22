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
#include "config/tx.h"

//Duplicated in channels.c
#define SWITCH_3x2  0
#define SWITCH_2x2  ((1 << INP_SWA2) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_NONE ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))

u32 global_extra_switches = 0;
static const u16 columns[] = {GPIO5, GPIO6, GPIO7, GPIO8, 0xffff};
static const u16 rows[] = {GPIO6, GPIO7, GPIO8, GPIO9, 0xffff};
static const u8 buttonmap[] = {
//         C.6              C.7              C.8              C.9
/*B.5*/    BUT_TRIM_RH_POS, BUT_TRIM_RH_NEG, BUT_TRIM_RV_POS, BUT_TRIM_RV_NEG,
/*B.6*/    BUT_LAST,        BUT_ENTER,       BUT_RIGHT,       BUT_LEFT,
/*B.7*/    BUT_TRIM_LV_POS, BUT_TRIM_LV_NEG, BUT_TRIM_LH_NEG, BUT_TRIM_LH_POS,
/*B.8*/    BUT_LAST+1,      BUT_DOWN,        BUT_UP,          BUT_EXIT,
    };

// Extra switch connections
//         2x2              3x1              3x2
//         -----            -----            -----
//B.5                                        SW_B0
//B.6      SW_B1            SW_A0            SW_B2
//B.7                                        SW_A0
//B.8      SW_A1            SW_A2            SW_A2
//global_extra_switches:
//  .0 == B0
//  .1 == B2
//  .2 == A0
//  .3 == A2

#define COL_PORT GPIOB
#define COL_PORT_MASK (GPIO5 | GPIO6 | GPIO7 | GPIO8)
#define ROW_PORT GPIOC


void Initialize_ButtonMatrix()
{
  /* Enable AFIO */
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_AFIOEN);

  /* Remap GPIO_Remap_SWJ_JTAGDisable */
  AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

  /* Enable GPIOB & GPIOE */
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
  rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
  
  /* PortB 5, 6, 7, 8 are open-drain output */
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
                GPIO5 | GPIO6 | GPIO7 | GPIO8);

  gpio_set(GPIOB, GPIO5 | GPIO6| GPIO7 | GPIO8);

  /* PortC 6-9 are pull-up inputs */
  gpio_set_mode(GPIOC, GPIO_MODE_INPUT,
                GPIO_CNF_INPUT_PULL_UPDOWN, GPIO6 | GPIO7 | GPIO8 | GPIO9);
  gpio_set(GPIOC, GPIO6 | GPIO7 | GPIO8 | GPIO9);
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
            if(!(but & *r)) {
                result |= 1 << (buttonmap[idx] - 1);
            }
            idx++;
        }
    }
    if (Transmitter.ignore_src != SWITCH_NONE) {
        //Write to C.6, read B
        if (result == 0) {
            gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO6);
            gpio_clear(GPIOC, GPIO6);
            u32 port = gpio_port_read(GPIOB);
            gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO6);
            gpio_set(GPIOC, GPIO6);
            if (Transmitter.ignore_src == SWITCH_3x1) {
                global_extra_switches = (((~port) >> 4) & 0x04) | (((~port) >> 5) & 0x08);
            } else if (Transmitter.ignore_src == SWITCH_2x2) {
                global_extra_switches  = (port>>6)&0x05;
            } else {
                global_extra_switches  = (~(port>>5))&0xf;
            }
        }
    }
    return result;
}
