#ifndef _DEVO10_HARDWARE_H_
#define _DEVO10_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define BUTTON_MATRIX { \
/*          PE.2             PE.3             PE.4             PE.5             PE.6         */  \
/*PB.6*/    BUT_LEFT,        BUT_RIGHT,       BUT_ENTER,       BUT_TRIM_RH_NEG, BUT_TRIM_RH_POS, \
/*PB.7*/    BUT_DOWN,        BUT_UP,          BUT_EXIT,        BUT_TRIM_LH_POS, BUT_TRIM_LH_NEG, \
/*PB.8*/    BUT_LAST,        BUT_TRIM_R_NEG,  BUT_TRIM_R_POS,  BUT_TRIM_L_POS,  BUT_TRIM_L_NEG,  \
/*PB.9*/    BUT_TRIM_LV_NEG, BUT_TRIM_RV_POS, BUT_TRIM_LV_POS, BUT_LAST,        BUT_TRIM_RV_NEG, \
    }
#define BUTTON_MATRIX_ROW_OD ((struct mcu_pin){GPIOB, GPIO6 | GPIO7 | GPIO8 | GPIO9})
#define BUTTON_MATRIX_COL_PU ((struct mcu_pin){GPIOE, GPIO2 | GPIO3 | GPIO4 | GPIO5 | GPIO6})

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 - INP_AIL */ \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 - INP_ELE */ \
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 - INP_THR */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 - INP_RUD */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    }

#endif  // _DEVO10_HARDWARE_H_
