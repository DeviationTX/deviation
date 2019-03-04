#ifndef _DTX_STM32_GPIO_H_
#define _DTX_STM32_GPIO_H_

struct mcu_pin {
    u32 port;
    u16 pin;
};

struct spi_config {
    unsigned spi;
    struct mcu_pin sck;
    struct mcu_pin mosi;
    struct mcu_pin miso;
    unsigned rate;
    unsigned cpol;
    unsigned cpha;
    unsigned dff;
    unsigned endian;
    unsigned altfn;
};

#define DEFAULT_SPI_SETTINGS \
    .cpol = SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE, \
    .cpha = SPI_CR1_CPHA_CLK_TRANSITION_1, \
    .dff  = SPI_CR1_DFF_8BIT, \
    .endian = SPI_CR1_MSBFIRST

struct spi_csn {
    unsigned spi;
    struct mcu_pin csn;
};

struct adc_config {
    unsigned adc;
    unsigned prescalar;
    unsigned sampletime;
};

struct i2c_config {
    unsigned i2c;
    struct mcu_pin scl_sca;
    unsigned i2c_freq;        // Target i2c frequency (Hz) (max: std-mode:100kHz, fast-mode:400kHz)
    unsigned dutycycle;       // only relevant for fast-mode I2C_CCR_DUTY_DIV2 or I2C_CCR_DUTY_16_DIV_9
    unsigned fastmode;        // 1: fast-mode, 0: standard-mode
};

struct uart_config {
    unsigned uart;
    struct mcu_pin tx;
    struct mcu_pin rx;
};

struct tim_config {
    unsigned tim;
    struct mcu_pin pin;
    unsigned ch;          // Timer channel
    unsigned chn;         // Used for CH1N/CH2N
};

struct dma_config {
    unsigned dma;
    unsigned stream;
    unsigned channel;
};

#define NULL_PIN ((struct mcu_pin){0, 0})
#define HAS_PIN(x) (x.port)


#if defined(STM32F1)
    #include "f1/gpio.h"
#elif defined(STM32F2)
    #include "f2/gpio.h"
#endif

#endif  // _DTX_STM32_GPIO_H_
