#ifndef _DEVO12_HARDWARE_H_
#define _DEVO12_HARDWARE_H_

#include "target/drivers/mcu/stm32/gpio.h"

#define BUTTON_MATRIX { \
/*         B.5              B.6              B.7              B.8              B.9          */  \
/*C.6*/    BUT_LAST,        BUT_UP,          BUT_DOWN,        BUT_EXIT,        BUT_TRIM_LH_POS, \
/*C.7*/    BUT_RIGHT,       BUT_LEFT,        BUT_ENTER,       BUT_LAST,        BUT_TRIM_LH_NEG, \
/*C.8*/    BUT_TRIM_L_POS,  BUT_TRIM_L_NEG,  BUT_TRIM_R_POS,  BUT_TRIM_R_NEG,  BUT_TRIM_LV_NEG, \
/*C.9*/    BUT_TRIM_RV_POS, BUT_TRIM_RH_POS, BUT_TRIM_RH_NEG, BUT_TRIM_RV_NEG, BUT_TRIM_LV_POS, \
    }

#define BUTTON_MATRIX_ROW_OD ((struct mcu_pin){GPIOC, GPIO6 | GPIO7 | GPIO8 | GPIO9})
#define BUTTON_MATRIX_COL_PU ((struct mcu_pin){GPIOB, GPIO5 | GPIO6 | GPIO7 | GPIO8 | GPIO9})

// Analog inputs
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOA, GPIO2),  /* ADC123_2  */ \
    ADC_CHAN(GPIOC, GPIO1),  /* ADC123_11 */ \
    ADC_CHAN(GPIOC, GPIO0),  /* ADC123_10 */ \
    ADC_CHAN(GPIOF, GPIO10), /* ADC3_8    */ \
    ADC_CHAN(GPIOF, GPIO7),  /* ADC3_5    */ \
    ADC_CHAN(GPIOC, GPIO2),  /* ADC123_12 */ \
    ADC_CHAN(GPIOF, GPIO8),  /* ADC3_6    */ \
    ADC_CHAN(GPIOC, GPIO3),  /* ADC123_13 */ \
    ADC_CHAN(GPIOF, GPIO6),  /* ADC3_4    */ \
    ADC_CHAN(GPIOF, GPIO9),  /* ADC3_7    */ \
    ADC_CHAN(0, 0),        /* TEMPERATURE */ \
    ADC_CHAN(GPIOA, GPIO3),  /* ADC123_3  */ \
    }

// ADC overrides
#define ADC_OVERSAMPLE_WINDOW_COUNT 10
#define ADC_CFG ((struct adc_config) {       \
    .adc = ADC3,                             \
    .prescalar = RCC_CFGR_ADCPRE_PCLK2_DIV6, \
    .sampletime = ADC_SMPR_SMP_13DOT5CYC,   \
    })
#define ADC_DMA ((struct dma_config) { \
    .dma = DMA2,                       \
    .stream = DMA_CHANNEL5,            \
    })

// PWM overrides
#define PWM_TIMER ((struct tim_config) { \
    .tim = TIM1,           \
    .pin = {GPIOA, GPIO8}, \
    .ch = 1,               \
    })
#define PWM_DMA ((struct dma_config) { \
    .dma = DMA1,                       \
    .stream = DMA_CHANNEL2,            \
    })
#define _PWM_DMA_ISR                dma1_channel2_isr

// Touch pins
#define TOUCH_SPI ((struct spi_csn) { \
    .spi = SPI1,                      \
    .csn = {GPIOC, GPIO4},            \
    })
#define TOUCH_IRQ_PIN ((struct mcu_pin){GPIOC, GPIO5})
#define TOUCH_COORDS_REVERSE      0
#define TOUCH_SPI_CFG SPI1_CFG

// LCD override
#define LCD_SPI ((struct spi_csn) { \
    .spi = SPI1,                    \
    .csn = {GPIOB, GPIO1},          \
    })
#define LCD_SPI_CFG SPI1_CFG

// Backlight override
#define BACKLIGHT_TIM ((struct tim_config) { \
    .tim = TIM5,            \
    .pin = {GPIOA, GPIO0},  \
    .ch = 1,                \
    })

// Sound override
#define SOUND_TIM ((struct tim_config) { \
    .tim = TIM3,           \
    .pin = {GPIOB, GPIO0}, \
    .ch = 3,               \
    })

// Power switch override
#define PWR_SWITCH_PIN ((struct mcu_pin) {GPIOA, GPIO4})
#define PWR_ENABLE_PIN ((struct mcu_pin) {GPIOE, GPIO0})

#endif  // _DEVO12_HARDWARE_H_
