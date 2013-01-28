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
#include <libopencm3/cm3/scb.h>
#include "common.h"
#include "../common_devo/devo.h"

/* Return milivolts */
u16 PWR_ReadVoltage(void)
{
    u32 v = adc_array_raw[4];
    /* Compute voltage from y = 0.0021x + 0.3026 */
    /* Multily the above by 1000 to get milivolts */
    v = v * 21 / 10 + 303;
    return v;
}
