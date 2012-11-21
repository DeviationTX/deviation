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
#include "devo.h"
#include "config/tx.h"

void VIBRATINGMOTOR_Init()
{
    if (!HAS_VIBRATINGMOTOR)
        return;
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);

    gpio_set_mode(GPIOD, GPIO_MODE_OUTPUT_2_MHZ,
            GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);
    gpio_clear(GPIOD, GPIO2);
}

void VIBRATINGMOTOR_Start()
{
    if (!HAS_VIBRATINGMOTOR || !Transmitter.vibration_state)
        return;
    gpio_set(GPIOD, GPIO2);
}

void VIBRATINGMOTOR_Stop()
{
    if (!HAS_VIBRATINGMOTOR)
        return;
    gpio_clear(GPIOD, GPIO2);
}

