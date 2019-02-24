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
#include "target/drivers/mcu/stm32/rcc.h"

#define _BTN(...) {__VA_ARGS__}
static const struct mcu_pin btn_pins[BUT_LAST-1] = BUTTONS;
#undef _BTN

void Initialize_ButtonMatrix()
{
    #define _BTN(x, y) get_rcc_from_port(x)
    const uint32_t btn_rcc[BUT_LAST-1] = BUTTONS;
    #undef _BTN
    for (unsigned i = 0; i < BUT_LAST-1; i++) {
        rcc_periph_clock_enable(btn_rcc[i]);
        GPIO_setup_input(btn_pins[i], ITYPE_PULLUP);
    }
}

u32 ScanButtons()
{
    u32 result = 0;
    for (int i = 1; i < BUT_LAST; i++) {
        int val = !GPIO_pin_get(btn_pins[i-1]);
        if (val)
            result = 1 << (i-1);
    }
    return result;
}
