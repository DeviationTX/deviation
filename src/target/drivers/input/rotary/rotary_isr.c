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
#include "common.h"
#include "target/drivers/mcu/stm32/exti.h"
#include "target/drivers/mcu/stm32/nvic.h"

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

void __attribute__((__used__)) ROTARY_ISR()
{
    ctassert(ROTARY_PIN0.port == ROTARY_PIN1.port, rotary_pins_must_be_on_same_port);
    u32 port = gpio_port_read(ROTARY_PIN0.port);
    exti_reset_request(EXTIx(ROTARY_PIN0) | EXTIx(ROTARY_PIN1));
    u32 button = ((port & ROTARY_PIN0.pin) ? 0 : 0x01)
               | ((port & ROTARY_PIN1.pin) ? 0 : 0x02);
    int dir = handle_rotary_encoder(button);
    if (button == 0) {
        rotary = 0;
    } else {
        rotary = dir;
    }
}
