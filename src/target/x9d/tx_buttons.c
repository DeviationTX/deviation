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
#include <libopencm3/stm32/f2/rcc.h>
#include <libopencm3/stm32/f2/gpio.h>
#include "common.h"

void Initialize_ButtonMatrix()
{
  /* Enable GPIOC & GPIOE */
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPCEN);
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);
  rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPEEN);
  
  gpio_mode_setup(GPIOC, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                GPIO1 | GPIO2 | GPIO3 | GPIO13);

  gpio_mode_setup(GPIOD, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                GPIO2 | GPIO3 | GPIO7);

  gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP,
                GPIO3 | GPIO4 | GPIO5 | GPIO6 | GPIO10 | GPIO11 | GPIO12);
}
u32 ScanButtons()
{
    u32 result = 0;
    for(int i = 1; i < BUT_LAST; i++) {
        int val = 0;
        switch(i) {
            case BUT_TRIM_LV_NEG: val = ! gpio_get(GPIOE, GPIO6); break;
            case BUT_TRIM_LV_POS: val = ! gpio_get(GPIOE, GPIO5); break;
            case BUT_TRIM_RV_NEG: val = ! gpio_get(GPIOC, GPIO1); break;
            case BUT_TRIM_RV_POS: val = ! gpio_get(GPIOC, GPIO13); break;
            case BUT_TRIM_LH_NEG: val = ! gpio_get(GPIOE, GPIO6); break;
            case BUT_TRIM_LH_POS: val = ! gpio_get(GPIOE, GPIO5); break;
            case BUT_TRIM_RH_NEG: val = ! gpio_get(GPIOC, GPIO3); break;
            case BUT_TRIM_RH_POS: val = ! gpio_get(GPIOC, GPIO2); break;
            case BUT_LEFT:        val = ! gpio_get(GPIOE, GPIO12); break; //-
            case BUT_RIGHT:       val = ! gpio_get(GPIOE, GPIO10); break; //+
            case BUT_UP:          val = ! gpio_get(GPIOD, GPIO7); break; //Menu
            case BUT_DOWN:        val = ! gpio_get(GPIOD, GPIO3); break; //Page
            case BUT_ENTER:       val = ! gpio_get(GPIOE, GPIO11); break;
            case BUT_EXIT:        val = ! gpio_get(GPIOD, GPIO2); break;
        }
        if (val)
            result = 1 << (i-1);
    }
    return result;
}
