#ifndef _DEVO10_HARDWARE_H_
#define _DEVO10_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
    ADC_CHAN(GPIOC, GPIO5),  /* ADC12_15  */ \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
    ADC_CHAN(GPIOA, GPIO4),  /* ADC12_4   */ \
    ADC_CHAN(GPIOA, GPIO0),  /* ADC123_0  */ \
    ADC_CHAN(GPIOB, GPIO0),  /* ADC12_8   */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    }

#endif  // _DEVO10_HARDWARE_H_
