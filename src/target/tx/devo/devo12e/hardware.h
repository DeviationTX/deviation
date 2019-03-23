#ifndef _DEVO10_HARDWARE_H_
#define _DEVO10_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define BUTTON_MATRIX { \
/*          PE.2             PE.3             PE.4             PE.5             PE.6         */  \
/*PB.4*/    BUT_LEFT,        BUT_RIGHT,       BUT_ENTER,       BUT_TRIM_RH_POS, BUT_TRIM_RH_NEG, \
/*PB.5*/    BUT_DOWN,        BUT_UP,          BUT_EXIT,        BUT_TRIM_LH_NEG, BUT_TRIM_LH_POS, \
/*PB.8*/    BUT_LAST,        BUT_TRIM_R_POS,  BUT_TRIM_R_NEG,  BUT_TRIM_L_NEG,  BUT_TRIM_L_POS,  \
/*PB.9*/    BUT_TRIM_LV_NEG, BUT_TRIM_RV_POS, BUT_TRIM_LV_POS, BUT_LAST,        BUT_TRIM_RV_NEG, \
    }
#define BUTTON_MATRIX_ROW_OD ((struct mcu_pin){GPIOB, GPIO4 | GPIO5 | GPIO8 | GPIO9})
#define BUTTON_MATRIX_COL_PU ((struct mcu_pin){GPIOE, GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6})

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO2, CHAN_NONINV),  /* ADC123_12 - INP_AIL */ \
    ADC_CHAN(GPIOC, GPIO5, CHAN_NONINV),  /* ADC12_15  - INP_ELE */ \
    ADC_CHAN(GPIOC, GPIO3, CHAN_INVERT),  /* ADC123_13 - INP_THR */ \
    ADC_CHAN(GPIOC, GPIO1, CHAN_INVERT),  /* ADC123_11 - INP_RUD */ \
    ADC_CHAN(GPIOC, GPIO0, CHAN_NONINV),  /* ADC123_10 - INP_AUX4 */ \
    ADC_CHAN(GPIOB, GPIO0, CHAN_NONINV),  /* ADC12_8   - INP_AUX5 */ \
    ADC_CHAN(GPIOA, GPIO4, CHAN_NONINV),  /* ADC12_4   - INP_AUX6 */ \
    ADC_CHAN(GPIOA, GPIO0, CHAN_NONINV),  /* ADC123_0  - INP_AUX7 */ \
    ADC_CHAN(0, 16, CHAN_NONINV),         /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4, CHAN_INVERT),  /* ADC12_14  */ \
    }

#endif  // _DEVO10_HARDWARE_H_
