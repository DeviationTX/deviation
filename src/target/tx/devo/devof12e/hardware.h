#ifndef _DEVOF12E_HARDWARE_H_
#define _DEVOF12E_HARDWARE_H_

#define BUTTON_MATRIX { \
/*         E.2              E.3              E.4              E.5              E.6          */  \
/*PB.4*/   BUT_LEFT,        BUT_TRIM_RH_POS, BUT_TRIM_RH_NEG, BUT_ENTER,       BUT_RIGHT,       \
/*PB.5*/   BUT_TRIM_LH_NEG, BUT_TRIM_LH_POS, BUT_EXIT,        BUT_UP,          BUT_DOWN,        \
/*PB.6*/   BUT_TRIM_L_POS,  BUT_TRIM_L_NEG,  BUT_TRIM_R_NEG,  BUT_TRIM_R_POS,  BUT_LAST,        \
/*PB.7*/   BUT_LAST,        BUT_TRIM_LV_POS, BUT_TRIM_RV_POS, BUT_TRIM_LV_NEG, BUT_TRIM_RV_NEG, \
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
    ADC_CHAN(GPIOB, GPIO0),  /* ADC12_8   */ \
    ADC_CHAN(GPIOA, GPIO4),  /* ADC12_4   */ \
    ADC_CHAN(GPIOA, GPIO0),  /* ADC123_0  */ \
    ADC_CHAN(0, 16),       /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO4),  /* ADC12_14  */ \
    }

#define I2C1_CFG ((struct i2c_config) { \
    .i2c = I2C1,                        \
    .scl_sca = {GPIOB, GPIO6 | GPIO7},  \
    .i2c_freq = 400000,                 \
    .dutycycle = I2C_CCR_DUTY_DIV2,     \
    .fastmode  = 1,                     \
    })
#define LCD_I2C_CFG I2C1_CFG

#define LCD_RESET_PIN ((struct mcu_pin){GPIOE, GPIO7})

#define LCD_VIDEO_CS0 ((struct mcu_pin) {GPIOE, GPIO8})
#define LCD_VIDEO_CS1 ((struct mcu_pin) {GPIOE, GPIO9})
#define LCD_VIDEO_CS2 ((struct mcu_pin) {GPIOE, GPIO10})
#define LCD_VIDEO_CS3 ((struct mcu_pin) {GPIOD, GPIO10})
#define LCD_VIDEO_CS4 ((struct mcu_pin) {GPIOD, GPIO8})
#define LCD_VIDEO_PWR ((struct mcu_pin){GPIOE, GPIO11})
#endif
