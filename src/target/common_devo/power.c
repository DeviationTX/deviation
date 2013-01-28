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

void PWR_Init(void)
{
    SCB_VTOR = VECTOR_TABLE_LOCATION;
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT; //sleep immediate on WFI
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    /* Enable GPIOA so we can manage the power switch */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);

    /* Pin 2 controls power-down */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO2);

    /* Enable GPIOA.2 to keep from shutting down */
    gpio_set(GPIOA, GPIO2);

    /* When Pin 3 goes high, the user turned off the Tx */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO3);

    /* Enable Voltage measurement */
    /* ADC is configured by the 'channel' code */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO4);
}

void PWR_Shutdown()
{
    printf("Shutdown\n");
    BACKLIGHT_Brightness(0);
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSICLK);
    rcc_wait_for_osc_ready(HSI);
    gpio_clear(GPIOA, GPIO2);
    while(1) ;
}

int PWR_CheckPowerSwitch()
{
    //static u16 debounce = 0;
    if(gpio_get(GPIOA, GPIO3)) {
/*
        debounce++;
    } else {
        debounce = 0;
    }
    if(debounce >= 0x40) {
*/
        return 1;
    }
    return 0;
}

void PWR_Sleep()
{
    asm("wfi");
}
