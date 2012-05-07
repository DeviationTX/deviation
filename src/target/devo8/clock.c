/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "target.h"

#include <libopencm3/stm32/systick.h>

u32 msecs;

void CLOCK_Init()
{
    /* 72MHz / 8 => 9000000 counts per second */
    systick_set_clocksource(STK_CTRL_CLKSOURCE_AHB_DIV8);

    /* 9000000/9000 = 1000 overflows per second - every 1ms one interrupt */
    systick_set_reload(9000);

    systick_interrupt_enable();

    msecs = 0;
    /* Start counting. */
    systick_counter_enable();
}

u32 CLOCK_getms()
{
    return msecs;
}

void sys_tick_handler(void)
{
	msecs++;
}
