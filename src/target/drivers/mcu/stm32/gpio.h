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
    unsigned speed;
};

struct uart_config {
    unsigned uart;
    struct mcu_pin tx;
    struct mcu_pin rx;
};

struct tim_config {
    unsigned tim;
    unsigned rst;
    unsigned irq;
};

#define _TIM_CONCAT(x, y, z) x ## y ## z
#define TIM_CONCAT(x, y, z)  _TIM_CONCAT(x, y, z)
#define TIM_CFG(x) ((struct tim_config){ \
    .tim = TIM_CONCAT(TIM, x,),          \
    .rst = TIM_CONCAT(RST_TIM, x,),      \
    .irq = TIM_CONCAT(NVIC_TIM, x, _IRQ) \
    })

struct dma_config {
    unsigned dma;
    unsigned stream;
    unsigned channel;
};

#define NULL_PIN ((struct mcu_pin){0, 0})
#define HAS_PIN(x) (x.pin)


#if defined(STM32F1)
    #include "f1/gpio.h"
#elif defined(STM32F2)
    #include "f2/gpio.h"
#endif

#endif  // _DTX_STM32_GPIO_H_
