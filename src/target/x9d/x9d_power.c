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
#include <libopencm3/stm32/f2/gpio.h>
#include <libopencm3/stm32/f2/rcc.h>
#include <libopencm3/cm3/scb.h>
#include "common.h"
#include "../common_devo/devo.h"

void PWR_Init(void)
{
    //SCB_VTOR = VECTOR_TABLE_LOCATION;
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT; //sleep immediate on WFI
    //rcc_clock_setup_in_hse_8mhz_out_72mhz();

    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPCEN);
    rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_IOPDEN);

    /* PD.0 controls power-down */
    gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, GPIO0);
    gpio_set(GPIOD, GPIO0);

    /* PD.1 is power switch */
    gpio_mode_setup(GPIOD, GPIO_MODE_INPUT,
                  GPIO_PUPD_PULLUP, GPIO1);

    /* PC.6 enables the LED */
    gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, GPIO6);
    gpio_set(GPIOC, GPIO6);
}

void PWR_Shutdown()
{
    printf("Shutdown\n");
    gpio_clear(GPIOD, GPIO0);
    while(1) ;
}

int PWR_CheckPowerSwitch()
{
    //static u16 debounce = 0;
    if(! gpio_get(GPIOD, GPIO1)) {
        return 1;
    }
    return 0;
}

void PWR_Sleep()
{
    asm("wfi");
}

/* Return milivolts */
u16 PWR_ReadVoltage(void)
{
    u32 v = adc_array_raw[NUM_ADC_CHANNELS-1];
    /* Multily the above by 1000 to get milivolts */
    v = v * VOLTAGE_NUMERATOR / 100 + VOLTAGE_OFFSET;
    return v;
}
