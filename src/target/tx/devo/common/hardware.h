#ifndef _DEVO_DEFAULT_HARDWARE_H_
#define _DEVO_DEFAULT_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#ifndef ADC_CFG
  #define ADC_CFG ((struct adc_config) {       \
      .adc = ADC1,                             \
      .prescalar = RCC_CFGR_ADCPRE_PCLK2_DIV6, \
      .sampletime = ADC_SMPR_SMP_239DOT5CYC,   \
      })
#endif
#endif  // _DEVO_DEFAULT_HARDWARE_H_
