#ifndef _DTX_STM32F2_ADC_H_
#define _DTX_STM32F2_ADC_H_

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>

static inline void ADC_reset(uint32_t adc)
{
    (void)adc;
    rcc_periph_reset_pulse(RST_ADC);
}
static void ADC_start_conversion(uint32_t adc)
{
    adc_start_conversion_regular(adc);
}

static void ADC_calibrate(uint32_t adc)
{
    /// STM32F2 auoto-calibrates
    (void)adc;
}

#endif  // _DTX_STM32F2_ADC_H_
