#ifndef _HARDWARE_H_
#define _HARDWARE_H_

// included from common.h (don't include common.h here!)
#include "target/drivers/mcu/stm32/gpio.h"

#define HAS_BUTTON_POWER_ON 1
#define HAS_JR_SERIAL_MODULE 1
#define HAS_SSER_TX 1

// Buttons
// NOTE: Button order must match capabilities.h
#define BUTTONS { \
    _BTN(GPIOE, GPIO6),  /*BUT_TRIM_LV_NEG*/ \
    _BTN(GPIOE, GPIO5),  /*BUT_TRIM_LV_POS*/ \
    _BTN(GPIOC, GPIO3),  /*BUT_TRIM_RV_NEG*/ \
    _BTN(GPIOC, GPIO2),  /*BUT_TRIM_RV_POS*/ \
    _BTN(GPIOD, GPIO15), /*BUT_TRIM_LH_NEG*/ \
    _BTN(GPIOC, GPIO1),  /*BUT_TRIM_LH_POS*/ \
    _BTN(GPIOE, GPIO3),  /*BUT_TRIM_RH_NEG*/ \
    _BTN(GPIOE, GPIO4),  /*BUT_TRIM_RH_POS*/ \
    _BTN(GPIOD, GPIO7),  /*BUT_LEFT*/        \
    _BTN(GPIOD, GPIO3),  /*BUT_RIGHT*/       \
    _BTN(GPIOE, GPIO11), /*BUT_DOWN*/        \
    _BTN(GPIOE, GPIO9),  /*BUT_UP*/          \
    _BTN(GPIOE, GPIO10), /*BUT_ENTER*/       \
    _BTN(GPIOD, GPIO2),  /*BUT_EXIT*/        \
    }

// Analog channels
#define ADC_CHANNELS { \
    ADC_CHAN(GPIOA, GPIO1, CHAN_INVERT),  /* ADC123_1 - INP_AIL */ \
    ADC_CHAN(GPIOA, GPIO2, CHAN_NONINV),  /* ADC123_2 - INP_ELE */ \
    ADC_CHAN(GPIOA, GPIO0, CHAN_INVERT),  /* ADC123_0 - INP_THR */ \
    ADC_CHAN(GPIOA, GPIO3, CHAN_NONINV),  /* ADC123_3 - INP_RUD */ \
    ADC_CHAN(GPIOB, GPIO0, CHAN_INVERT),  /* ADC12_8  - INP_AUX4 */ \
    ADC_CHAN(GPIOA, GPIO6, CHAN_NONINV),  /* ADC12_6  - INP_AUX5 */ \
    ADC_CHAN(0, 16, CHAN_NONINV),         /* TEMPERATURE */ \
    ADC_CHAN(GPIOC, GPIO0, CHAN_NONINV),  /* ADC123_10 */ \
    }

#define SWITCHES \
    THREE_WAY(INP_SWA, (GPIOE, GPIO2), (GPIOE, GPIO13), CHAN_INVERT) \
    THREE_WAY(INP_SWB, (GPIOE, GPIO15), (GPIOA, GPIO5), CHAN_INVERT) \
    THREE_WAY(INP_SWC, (GPIOD, GPIO11), (GPIOE, GPIO0), CHAN_INVERT) \
    THREE_WAY(INP_SWD, (GPIOE, GPIO1), (GPIOE, GPIO2), CHAN_INVERT) \
    TWO_WAY(INP_SWF, (GPIOE, GPIO14), CHAN_INVERT) \
    TWO_WAY(INP_SWH, (GPIOD, GPIO14), CHAN_INVERT)

// Power switch configuration
#define PWR_SWITCH_PIN   ((struct mcu_pin){GPIOD, GPIO1})
#define PWR_ENABLE_PIN   ((struct mcu_pin){GPIOD, GPIO0})

// LEDs
#define LED_RED_PIN   ((struct mcu_pin){GPIOC, GPIO5})
#define LED_GREEN_PIN ((struct mcu_pin){GPIOB, GPIO1})
#define LED_BLUE_PIN  ((struct mcu_pin){GPIOC, GPIO4})
#define LED_STATUS_PIN     LED_BLUE_PIN
#define LED_RF_PIN         LED_GREEN_PIN
#define LED_STORAGE_PIN    LED_RED_PIN

// Haptic
#define HAPTIC_PIN ((struct mcu_pin){GPIOB, GPIO8})


// LCD uses SPI3
// On STM32F2, LCD is on SPI3->APB1@30MHz ClockDiv=2, Fspi=15MHz
// On STM32F1, LCD is on SPI1->APB2@72MHz, ClockDiv=4, Fspi=18MHz
#define SPI3_CFG ((struct spi_config) { \
    .spi = SPI3,            \
    .sck  = {GPIOC, GPIO10}, \
    .miso = NULL_PIN, \
    .mosi = {GPIOC, GPIO12}, \
    .rate = SPI_CR1_BAUDRATE_FPCLK_DIV_2, \
    DEFAULT_SPI_SETTINGS, \
    .altfn = 0 })

// SPI LCD
// Setup CS as A.15 Data/Control = C.11
#define LCD_SPI ((struct spi_csn) { SPI3, {GPIOA, GPIO15}})
#define LCD_SPI_MODE ((struct mcu_pin) {GPIOC, GPIO11})
#define LCD_SPI_CFG SPI3_CFG

// Setup ADC
// prescalar: APB2@60MHz/6 = 10MHz
#define ADC_OVERSAMPLE_WINDOW_COUNT  1
#define ADC_CFG ((struct adc_config){     \
    .adc = ADC1,                       \
    .prescalar = ADC_CCR_ADCPRE_BY6,   \
    .sampletime = ADC_SMPR_SMP_144CYC, \
    })

#define ADC_DMA ((struct dma_config) { \
    .dma = DMA2,                       \
    .stream = DMA_STREAM0,             \
    .channel = DMA_SxCR_CHSEL_0,       \
    })

// Backlight
#define BACKLIGHT_TIM ((struct tim_config) { \
    .tim = TIM4,              \
    .pin =  {GPIOD, GPIO13},  \
    .ch = 2,                  \
    })

#define SYSCLK_TIM ((struct tim_config) { \
    .tim = TIM5,   \
    .ch = 1        \
    })
#define SYSCLK_TIMER_ISR tim5_isr

// On STM32F2, SPI2->APB1@30MHz ClockDiv=2, Fspi=15MHz
// Keep SPI clock < 20MHz
#define SPI2_CFG ((struct spi_config) { \
    .spi = SPI2,            \
    .sck  = {GPIOB, GPIO13}, \
    .miso = {GPIOB, GPIO14}, \
    .mosi = {GPIOB, GPIO15}, \
    .rate = SPI_CR1_BR_FPCLK_DIV_2, \
    DEFAULT_SPI_SETTINGS, \
    .altfn = 0 })
#define MMC_SPI_CFG SPI2_CFG
#define MMC_SPI ((struct spi_csn) { SPI2, {GPIOB, GPIO12}})
#define MMC_PRESENT ((struct mcu_pin) {GPIOD, GPIO9})
#define MMC_PROTECT NULL_PIN
#define MMC_BAUDRATE_SLOW       SPI_CR1_BR_FPCLK_DIV_128
#define MMC_BAUDRATE_FAST       SPI_CR1_BR_FPCLK_DIV_2
#define MMC_RX_DMA ((struct dma_config) { \
    .dma = DMA1,                  \
    .stream = DMA_STREAM3,        \
    .channel = DMA_SxCR_CHSEL_0,  \
    })
#define MMC_TX_DMA ((struct dma_config) { \
    .dma = DMA1,                  \
    .stream = DMA_STREAM4,        \
    .channel = DMA_SxCR_CHSEL_0,  \
    })

// JREXT Module
#define SSER_TX_TIM ((struct tim_config) { \
    .tim = TIM8,                           \
    .pin = {GPIOA, GPIO7},                 \
    .ch = 1,                               \
    .chn = 1,                              \
    })
#define SSER_TX_DMA ((struct dma_config) { \
    .dma = DMA2,                         \
    .stream = DMA_STREAM1,               \
    .channel = DMA_SxCR_CHSEL_7,         \
    })
#define SSER_TX_DMA_ISR dma2_stream1_isr

#define SSER_TIM ((struct tim_config) { \
    .tim = TIM6,                        \
    .pin = {GPIOA, GPIO10},             \
    })
#define PWM_TIMER ((struct tim_config) { \
    .tim = TIM1,             \
    .pin = {GPIOA, GPIO9},   \
    .ch = 2,                 \
    })
#define PWM_TIMER_ISR exti9_5_isr    // Matches PA.9

// Audio DAC
#define AUDIODAC_DMA ((struct dma_config) { \
    .dma = DMA1,                  \
    .stream = DMA_STREAM5,        \
    .channel = DMA_SxCR_CHSEL_7,  \
    })

#define AUDIODAC_PIN ((struct mcu_pin) {GPIOA, GPIO4})
#define I2C_CFG ((struct i2c_config) { \
    .i2c = I2C1,                \
    .scl_sca = {GPIOB, GPIO6 | GPIO7},      \
    .speed = i2c_speed_sm_100k, \
    })
#define I2C_ADDRESS_VOLUME 0x5C

#define PROTO_SPI_CFG ((struct spi_config) {})

#define AUDIODAC_TIM TIM_CFG(2)  // TIM2
#include "target/drivers/mcu/stm32/hardware.h"

#endif  // _HARDWARE_H_

