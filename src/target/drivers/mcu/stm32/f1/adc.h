#ifndef _DTX_STM32F1_ADC_H_
#define _DTX_STM32F1_ADC_H_

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>

static inline void ADC_reset(uint32_t adc)
{
    int reset = (adc == ADC1) ? RCC_APB2RSTR_ADC1RST
              : (adc == ADC2) ? RCC_APB2RSTR_ADC2RST
              : RCC_APB2RSTR_ADC3RST;
    rcc_peripheral_reset(&RCC_APB2RSTR, reset);
    rcc_peripheral_clear_reset(&RCC_APB2RSTR, reset);
}

// Emulate newer libopencm3 interface
static inline void adc_set_clk_prescale(uint32_t prescalar)
{
    rcc_set_adcpre(prescalar);
}

static inline void adc_set_dma_continue(uint32_t adc)
{
    // Not implemented on STM32F1
    (void)adc;
}

static inline void ADC_start_conversion(uint32_t adc)
{
    adc_start_conversion_direct(adc);
}

static inline void ADC_calibrate(uint32_t adc)
{
    adc_reset_calibration(adc);
    adc_calibrate(adc);
}
#endif  // _DTX_STM32F1_ADC_H_
