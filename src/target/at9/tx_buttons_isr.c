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
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

#include "common.h"

extern volatile int rotary;

/* returns change in encoder state (-1,0,1) */
static int handle_rotary_encoder(unsigned val)
{
  // static const s8 enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  // static const s8 enc_states[] = {0,0,0,0,0,0,0,-1,0,0,0,1,0,0,0,0};
  static unsigned old_AB = 0;

  old_AB <<= 2;          // remember previous state
  old_AB |= val & 0x03;  // add current state

  unsigned index = old_AB & 0x0f;
  if (index == 7) return -1;
  if (index == 11) return 1;
  return 0;
}

void exti15_10_isr()
{
    u32 button = gpio_port_read(GPIOC);
    exti_reset_request(EXTI13 | EXTI14);
    button = 0x03 & ((~button) >> 13);
    int dir = handle_rotary_encoder(button);
    if (button == 0) {
        rotary = 0;
    } else {
        rotary = dir;
    }
}
