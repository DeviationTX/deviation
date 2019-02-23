#ifndef _HARDWARE_BUTTONMATRIX_H_
#define _HARDWARE_BUTTONMATRIX_H_

#define BUTTON_MATRIX { \
/*         C.6              C.7              C.8              C.9              C.10   C.11 */ \
/*B.5*/    BUT_TRIM_RH_POS, BUT_TRIM_RH_NEG, BUT_TRIM_RV_POS, BUT_TRIM_RV_NEG, SW_07, SW_08,  \
/*B.6*/    SW_10,           BUT_ENTER,       BUT_RIGHT,       BUT_LEFT,        SW_05, SW_06,  \
/*B.7*/    BUT_TRIM_LV_POS, BUT_TRIM_LV_NEG, BUT_TRIM_LH_NEG, BUT_TRIM_LH_POS, SW_01, SW_02,  \
/*B.8*/    SW_09,           BUT_DOWN,        BUT_UP,          BUT_EXIT,        SW_03, SW_04,  \
    }

#define BUTTON_MATRIX_ROW_OD ((struct mcu_pin){GPIOB, GPIO5 | GPIO6 | GPIO7 | GPIO8})
#define BUTTON_MATRIX_COL_PU ((struct mcu_pin){GPIOC, GPIO6 | GPIO7 | GPIO8 | GPIO9 | GPIO10 | GPIO11})
#define EXTRA_SWITCH_COL_OD ((struct mcu_pin){GPIOC, GPIO6})
#endif  // _HARDWARE_BUTTONMATRIX_H_
