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
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/scb.h>
#include "common.h"
#include "../common_devo/devo.h"

/* Return milivolts */
u16 PWR_ReadVoltage(void)
{
    u32 v = ADC1_Read(14);
    /* Compute voltage from y = 0.003246x + 0.4208 */
    /* Multily the above by 1000 to get milivolts */
    v = v * 324 / 100 + 421;
    return v;
}
