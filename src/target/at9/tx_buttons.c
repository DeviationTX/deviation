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

void Initialize_ButtonMatrix()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPDEN);

    /* Mode and End */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO14 | GPIO15);
    gpio_set(GPIOA, GPIO14 | GPIO15);
    /* Trim */
    gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  GPIO3 | GPIO4 | GPIO5 | GPIO6);
    gpio_set(GPIOB, GPIO3 | GPIO4 | GPIO5 | GPIO6);
    /*Rotary */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  GPIO13 | GPIO14 | GPIO15);
    gpio_set(GPIOC, GPIO13 | GPIO14 | GPIO15);
    /* Trim */
    gpio_set_mode(GPIOD, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  GPIO2 | GPIO3 | GPIO12 | GPIO13);
    gpio_set(GPIOD, GPIO2 | GPIO3 | GPIO12 | GPIO13);
}

/* returns change in encoder state (-1,0,1) */
int handle_rotary_encoder(unsigned val)
{
  static s8 enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static unsigned old_AB = 0;
  /**/
  old_AB <<= 2;                   //remember previous state
  old_AB |= ( val & 0x03 );  //add current state
  return ( enc_states[( old_AB & 0x0f )]);
}
u32 ScanButtons()
{
    static u32 last_rotary;
    u32 result = 0;
    result |= ! gpio_get(GPIOB, GPIO6)  ? CHAN_ButtonMask(BUT_TRIM_LV_NEG) : 0;
    result |= ! gpio_get(GPIOB, GPIO5)  ? CHAN_ButtonMask(BUT_TRIM_LV_POS) : 0;
    result |= ! gpio_get(GPIOD, GPIO3)  ? CHAN_ButtonMask(BUT_TRIM_RV_NEG) : 0;
    result |= ! gpio_get(GPIOD, GPIO2)  ? CHAN_ButtonMask(BUT_TRIM_RV_POS) : 0;
    result |= ! gpio_get(GPIOD, GPIO12) ? CHAN_ButtonMask(BUT_TRIM_LH_NEG) : 0;
    result |= ! gpio_get(GPIOD, GPIO13) ? CHAN_ButtonMask(BUT_TRIM_LH_POS) : 0;
    result |= ! gpio_get(GPIOB, GPIO3)  ? CHAN_ButtonMask(BUT_TRIM_RH_NEG) : 0;
    result |= ! gpio_get(GPIOB, GPIO4)  ? CHAN_ButtonMask(BUT_TRIM_RH_POS) : 0;

    result |= ! gpio_get(GPIOA, GPIO14) ? CHAN_ButtonMask(BUT_DOWN) : 0; // MODE
    result |= ! gpio_get(GPIOC, GPIO15) ? CHAN_ButtonMask(BUT_ENTER) : 0;
    result |= ! gpio_get(GPIOA, GPIO15) ? CHAN_ButtonMask(BUT_EXIT) : 0;

    unsigned right = gpio_get(GPIOC, GPIO14) ? 0 : 1;
    unsigned left  = gpio_get(GPIOC, GPIO13) ? 0 : 1;
    unsigned rotary = (right << 1) | left;
    int dir = handle_rotary_encoder(rotary);
    if (rotary != 0) {
        if ( dir != 0) {
            last_rotary = dir > 0 ? CHAN_ButtonMask(BUT_DOWN) : CHAN_ButtonMask(BUT_UP);
        }
    } else {
        last_rotary = 0;
    }
    result |= last_rotary;
    return result;
}
