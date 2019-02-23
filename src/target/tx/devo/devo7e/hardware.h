#ifndef _DEVO7E_HARDWARE_H_
#define _DEVO7E_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define BUTTON_MATRIX { \
/*         C.6              C.7              C.8              C.9          */  \
/*B.5*/    BUT_TRIM_RH_POS, BUT_TRIM_RH_NEG, BUT_TRIM_RV_POS, BUT_TRIM_RV_NEG, \
/*B.6*/    SW_10,           BUT_ENTER,       BUT_RIGHT,       BUT_LEFT,        \
/*B.7*/    BUT_TRIM_LV_POS, BUT_TRIM_LV_NEG, BUT_TRIM_LH_NEG, BUT_TRIM_LH_POS, \
/*B.8*/    SW_09,           BUT_DOWN,        BUT_UP,          BUT_EXIT,        \
    }

#define BUTTON_MATRIX_ROW_OD ((struct mcu_pin){GPIOB, GPIO5 | GPIO6 | GPIO7 | GPIO8})
#define BUTTON_MATRIX_COL_PU ((struct mcu_pin){GPIOC, GPIO6 | GPIO7 | GPIO8 | GPIO9})
#define EXTRA_SWITCH_COL_OD ((struct mcu_pin){GPIOC, GPIO6})

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    }
#endif  // _DEVO7E_HARDWARE_H_
