#ifndef _DEVO7E256_HARDWARE_H_
#define _DEVO7E256_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"
#include "../common/hardware_t8sg_buttonmatrix.h"

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 - INP_AIL */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 - INP ELE */ \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 - INP_THR */ \
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 - INP_RUD */ \
    ADC_CHAN(GPIOA, GPIO0),  /* ADC123_0  - INP_AUX4 */ \
    ADC_CHAN(GPIOA, GPIO4),  /* ADC12_4   - INP_AUX5 */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    }
#endif  // _DEVO7E256_HARDWARE_H_
