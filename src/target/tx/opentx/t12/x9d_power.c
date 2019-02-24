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
#include <libopencmsis/core_cm3.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/cm3/scb.h>
#include "common.h"
#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/pwr.h"
#include "target/drivers/mcu/stm32/jtag.h"


void PWR_Init(void)
{
    _pwr_init();

    rcc_periph_clock_enable(get_rcc_from_pin(PWR_SWITCH_PIN));
    rcc_periph_clock_enable(get_rcc_from_pin(PWR_ENABLE_PIN));

    /* Pin controls power-down */
    GPIO_setup_output(PWR_ENABLE_PIN, OTYPE_PUSHPULL);
    /* Enable GPIOA.2 to keep from shutting down */
    GPIO_pin_set(PWR_ENABLE_PIN);

    rcc_periph_clock_enable(get_rcc_from_pin(LED_RED_PIN));
    rcc_periph_clock_enable(get_rcc_from_pin(LED_GREEN_PIN));
    rcc_periph_clock_enable(get_rcc_from_pin(LED_BLUE_PIN));
    GPIO_setup_output(LED_RED_PIN, OTYPE_PUSHPULL);
    GPIO_setup_output(LED_GREEN_PIN, OTYPE_PUSHPULL);
    GPIO_setup_output(LED_BLUE_PIN, OTYPE_PUSHPULL);
    GPIO_pin_set(LED_BLUE_PIN);
    GPIO_pin_clear(LED_GREEN_PIN);
    GPIO_pin_clear(LED_RED_PIN);

    if(! HAS_JTAG) {
        DisableJTAG();
    }
}

void PWR_Shutdown()
{
    printf("Shutdown\n");
    BACKLIGHT_Brightness(0);
    __disable_irq();
    GPIO_pin_clear(PWR_ENABLE_PIN);
    while (1)  {
        // We may be here a long time if user hold power button
        CLOCK_ResetWatchdog();
    }
}

int PWR_CheckPowerSwitch()
{
#if HAS_BUTTON_POWER_ON
    static u32 debounce = 0;
#endif

    if (!GPIO_pin_get(PWR_SWITCH_PIN)) {
#if HAS_BUTTON_POWER_ON
        if (debounce == 0) debounce = CLOCK_getms();
    } else {
        debounce = 0;
    }
    if (debounce && (CLOCK_getms() - debounce) >= 1000) {  // 1 sec
#endif
        return 1;
    }
    return 0;
}

void PWR_Sleep()
{
    asm("wfi");
}
