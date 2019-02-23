#ifndef _DEVO7E256_HARDWARE_H_
#define _DEVO7E256_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"
#include "../common/hardware_t8sg_buttonmatrix.h"

// Analog inputs
#if HAS_EXTRA_POTS
    #define ADC_CHANNELS { \
        ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
        ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
        ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
        ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
        ADC_CHAN(GPIOA, GPIO0),  /* ADC123_0  */ \
        ADC_CHAN(GPIOA, GPIO4),  /* ADC12_4   */ \
        ADC_CHAN(0, 16),       /* TEMPERATURE */ \
        ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
        }
#else
    #define ADC_CHANNELS { \
        ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
        ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
        ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
        ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
        ADC_CHAN(0, 16),       /* TEMPERATURE */ \
        ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
        }
#endif  // HAS_EXTRA_POTS

#endif  // _DEVO7E256_HARDWARE_H_
