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

// Analog inputs must be in same order as in CHANDEF
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO2, CHAN_NONINV),  /* ADC123_12 - INP_AIL */ \
    ADC_CHAN(GPIOC, GPIO1, CHAN_NONINV),  /* ADC123_11 - INP_ELE */ \
    ADC_CHAN(GPIOC, GPIO0, CHAN_INVERT),  /* ADC123_10 - INP_THR */ \
    ADC_CHAN(GPIOC, GPIO3, CHAN_INVERT),  /* ADC123_13 - INP_RUD */ \
    ADC_CHAN(0, 16, CHAN_NONINV),         /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4, CHAN_NONINV),  /* ADC12_14  */ \
    }

#define SWITCHES \
    TWO_WAY(INP_HOLD, (GPIOC, GPIO11), CHAN_INVERT) \
    TWO_WAY(INP_FMOD, (GPIOC, GPIO10), CHAN_INVERT)

#define EXTRA_SWITCHES \
    EXTRA_3WAY(INP_SWA, 0x04, 0x08, CHAN_NONINV, SWITCH_3x1) \
    EXTRA_3WAY(INP_SWB, 0x01, 0x02, CHAN_NONINV, SWITCH_3x2) \
    EXTRA_2WAY(INP_SWA, 0x04, 0x08, CHAN_INVERT, SWITCH_2x1) \
    EXTRA_2WAY(INP_SWB, 0x01, 0x02, CHAN_INVERT, SWITCH_2x2)

#define ADDON_SWITCH_CFG \
    ADDON_SWITCH("2x1", SWITCH_2x1, 0) \
    ADDON_SWITCH("2x2", SWITCH_2x2, 0) \
    ADDON_SWITCH("3x1", SWITCH_3x1, 0) \
    ADDON_SWITCH("3x2", SWITCH_3x2, 0)

#define SWITCH_3x2  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2))
#define SWITCH_2x2  ((1 << INP_SWA0) | (1 << INP_SWA1) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1))
#define SWITCH_2x1  ((1 << INP_SWA0) | (1 << INP_SWA1))
#define SWITCH_STOCK ((1 << INP_HOLD0) | (1 << INP_HOLD1) \
                    | (1 << INP_FMOD0) | (1 << INP_FMOD1))
#endif  // _DEVO7E_HARDWARE_H_
