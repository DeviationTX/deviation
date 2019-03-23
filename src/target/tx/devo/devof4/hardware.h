#ifndef _DEVOF7_HARDWARE_H_
#define _DEVOF7_HARDWARE_H_

#define BUTTON_MATRIX { \
/*         C.6              C.7              C.8              C.9         */   \
/*B.5*/    BUT_LEFT,        BUT_RIGHT,       BUT_ENTER,       BUT_LAST,        \
/*B.6*/    BUT_DOWN,        BUT_UP,          BUT_EXIT,        BUT_LAST+1,      \
/*B.7*/    BUT_TRIM_RH_NEG, BUT_TRIM_RH_POS, BUT_TRIM_LH_NEG, BUT_TRIM_LH_POS, \
/*B.8*/    BUT_TRIM_LV_NEG, BUT_TRIM_RV_NEG, BUT_TRIM_LV_POS, BUT_TRIM_RV_POS, \
    }

#define BUTTON_MATRIX_ROW_OD ((struct mcu_pin){GPIOB, GPIO5 | GPIO6 | GPIO7 | GPIO8})
#define BUTTON_MATRIX_COL_PU ((struct mcu_pin){GPIOC, GPIO6 | GPIO7 | GPIO8 | GPIO9})
#define EXTRA_SWITCH_COL_OD ((struct mcu_pin){GPIOC, GPIO6})

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOC, GPIO2, CHAN_NONINV),  /* ADC123_12 - INP_AIL */ \
    ADC_CHAN(GPIOC, GPIO1, CHAN_NONINV),  /* ADC123_11 - INP_ELE */ \
    ADC_CHAN(GPIOC, GPIO0, CHAN_INVERT),  /* ADC123_10 - INP_THR */ \
    ADC_CHAN(GPIOC, GPIO3, CHAN_INVERT),  /* ADC123_13 - INP_RUD */ \
    ADC_CHAN(0, 16, CHAN_NONINV),         /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4, CHAN_NONINV),  /* ADC12_14  */ \
    }

#define LCD_VIDEO_CS0 ((struct mcu_pin){GPIOA, GPIO0})
#define LCD_VIDEO_CS1 ((struct mcu_pin){GPIOA, GPIO8})
#define LCD_VIDEO_CS2 ((struct mcu_pin){GPIOA, GPIO15})
#define LCD_VIDEO_PWR ((struct mcu_pin){GPIOB, GPIO9})

#endif  // _DEVOF7_HARDWARE_H_
