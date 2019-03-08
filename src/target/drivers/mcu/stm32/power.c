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
#include <libopencm3/cm3/scb.h>
#include "common.h"
#include "target/drivers/mcu/stm32/rcc.h"
#include "target/drivers/mcu/stm32/pwr.h"
#include "target/drivers/mcu/stm32/jtag.h"

#include "target/tx/devo/common/devo.h"  // FIXME - Shouldn't rely on things from 'devo' here

void PWR_Init(void)
{
    _pwr_init();

    /* Disable SWD and set SWD pins as I/O for programable switch */
#if !defined(USE_JTAG) || !USE_JTAG
    DisableJTAG();
#endif

    if (HAS_PIN(PWR_ENABLE_PIN)) {
        rcc_periph_clock_enable(get_rcc_from_pin(PWR_SWITCH_PIN));
        rcc_periph_clock_enable(get_rcc_from_pin(PWR_ENABLE_PIN));

        /* Pin controls power-down */
        GPIO_setup_output(PWR_ENABLE_PIN, OTYPE_PUSHPULL);
        /* Enable GPIOA.2 to keep from shutting down */
        GPIO_pin_set(PWR_ENABLE_PIN);

        /* When Pin goes high, the user turned off the Tx */
        GPIO_setup_input(PWR_SWITCH_PIN, ITYPE_FLOAT);
    }
}

void PWR_Shutdown()
{
    printf("Shutdown\n");
    BACKLIGHT_Brightness(0);
    _pwr_shutdown();

    if (HAS_PIN(PWR_ENABLE_PIN))
        GPIO_pin_clear(PWR_ENABLE_PIN);

    while (1) {
        // If the reset is held too long, the watchdog could kick in
        CLOCK_ResetWatchdog();
    }
}

int PWR_CheckPowerSwitch()
{
    if (HAS_PIN(PWR_SWITCH_PIN))
    {
        static u32 debounce = 0;

        u32 value = GPIO_pin_get(PWR_SWITCH_PIN);
        if ((HAS_PWR_SWITCH_INVERTED && !value)
            || (!HAS_PWR_SWITCH_INVERTED && value))
        {
            if (debounce == 0) {
                debounce = CLOCK_getms();
            }
        } else {
            debounce = 0;
        }

        if (debounce && (CLOCK_getms() - debounce) >= 100) {  // 0.25 sec
            return 1;
        }
    }

    return 0;
}

void PWR_Sleep()
{
    LED_Status(0);
    asm("wfi");
    LED_Status(1);
}

void PWR_JumpToProgrammer()
{
    scb_reset_system();
}
