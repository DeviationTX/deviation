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
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
    ADC_CHAN(GPIOC, GPIO5),  /* ADC12_15  */ \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
    ADC_CHAN(GPIOA, GPIO4),  /* ADC12_4   */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    }

#endif  // _DEVO10_HARDWARE_H_
