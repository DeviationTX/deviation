#ifndef _DTX_STM32_I2C_H_
#define _DTX_STM32_I2C_H_

#include <libopencm3/stm32/i2c.h>
#include "target/drivers/mcu/stm32/gpio.h"
#include "target/drivers/mcu/stm32/rcc.h"

static inline void _i2c_init(struct i2c_config i2c)
{
    unsigned ccr;
    unsigned max_trise;

    rcc_periph_clock_enable(get_rcc_from_port(i2c.i2c));
    i2c_peripheral_disable(i2c.i2c);
    i2c_set_clock_frequency(i2c.i2c, APB1_FREQ_MHz);
    if (i2c.fastmode) {
        max_trise = (3 * APB1_FREQ_MHz) / 10;  // APB * 300ns
        i2c_set_fast_mode(i2c.i2c);
        if (i2c.dutycycle == I2C_CCR_DUTY_DIV2) {
            ccr = (1000000 * APB1_FREQ_MHz) / (3 * i2c.i2c_freq);
        } else {
            ccr = (1000000 * APB1_FREQ_MHz) / ((16 + 9) * i2c.i2c_freq);
        }
    } else {
        max_trise = APB1_FREQ_MHz;  // APB * 1000ns
        i2c_set_standard_mode(i2c.i2c);
        ccr = (1000000 * APB1_FREQ_MHz) / (2 * i2c.i2c_freq);
    }
    i2c_set_ccr(i2c.i2c, 0xfff & ccr);
    i2c_set_dutycycle(i2c.i2c, i2c.dutycycle);
    i2c_set_trise(i2c.i2c, max_trise + 1);
    i2c_peripheral_enable(i2c.i2c);

    rcc_periph_clock_enable(get_rcc_from_pin(i2c.scl_sca));
    GPIO_setup_output_af(i2c.scl_sca, OTYPE_OPENDRAIN, i2c.i2c);

}

#endif  // _DTX_STM32_I2C_H_
