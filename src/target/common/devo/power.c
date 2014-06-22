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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/scb.h>
#include "common.h"
#include "devo.h"

void PWR_Init(void)
{
    SCB_VTOR = VECTOR_TABLE_LOCATION;
    SCB_SCR  &= ~SCB_SCR_SLEEPONEXIT; //sleep immediate on WFI
    rcc_clock_setup_in_hse_8mhz_out_72mhz();

    /* Enable GPIOA so we can manage the power switch */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, _PWREN_RCC_APB2ENR_IOPEN);
    rcc_peripheral_enable_clock(&RCC_APB2ENR, _PWRSW_RCC_APB2ENR_IOPEN);

    /* Pin 2 controls power-down */
    gpio_set_mode(_PWREN_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, _PWREN_PIN);

    /* Enable GPIOA.2 to keep from shutting down */
    gpio_set(_PWREN_PORT, _PWREN_PIN);

    /* When Pin 3 goes high, the user turned off the Tx */
    gpio_set_mode(_PWRSW_PORT, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, _PWRSW_PIN);

    /* Disable SWD and set SWD pins as I/O for programable switch */
    AFIO_MAPR = (AFIO_MAPR & ~AFIO_MAPR_SWJ_MASK) | AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF;
    gpio_set_mode(GPIO_BANK_JTMS_SWDIO, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO_JTMS_SWDIO);
    gpio_set(GPIO_BANK_JTMS_SWDIO, GPIO_JTMS_SWDIO);
    gpio_set_mode(GPIO_BANK_JTCK_SWCLK, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO_JTCK_SWCLK);
    gpio_set(GPIO_BANK_JTCK_SWCLK, GPIO_JTCK_SWCLK);
}

void PWR_Shutdown()
{
    printf("Shutdown\n");
    BACKLIGHT_Brightness(0);
    rcc_set_sysclk_source(RCC_CFGR_SW_SYSCLKSEL_HSICLK);
    rcc_wait_for_osc_ready(HSI);
    gpio_clear(_PWREN_PORT, _PWREN_PIN);
    while(1) ;
}

int PWR_CheckPowerSwitch()
{
    //static u16 debounce = 0;
    if(gpio_get(_PWRSW_PORT, _PWRSW_PIN)) {
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

/* Return milivolts */
unsigned PWR_ReadVoltage(void)
{
    u32 v = adc_array_raw[NUM_ADC_CHANNELS-1];
    /* Multily the above by 1000 to get milivolts */
    v = v * VOLTAGE_NUMERATOR / 100 + VOLTAGE_OFFSET;
    return v;
}
