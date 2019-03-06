#ifndef _DTX_STM32F2_GPIO_H_
#define _DTX_STM32F2_GPIO_H_

#include <libopencm3/stm32/gpio.h> /* For GPIO* definitions */
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/i2c.h>

#include "freq.h"

enum gpio_output_type {
    OTYPE_OPENDRAIN = GPIO_OTYPE_OD,
    OTYPE_PUSHPULL  = GPIO_OTYPE_PP,
};
enum gpio_input_type {
    ITYPE_ANALOG,
    ITYPE_FLOAT,
    ITYPE_PULLUP,
    ITYPE_PULLDOWN,
};

#define INLINE __attribute__((always_inline))
inline static unsigned _get_af(unsigned type)
{
    switch (type) {
        case TIM1:
        case GPIO_AF1:
            return GPIO_AF1;
        case TIM4:
        case GPIO_AF2:
            return GPIO_AF2;
        case TIM8:
        case GPIO_AF3:
            return GPIO_AF3;
        case I2C1:
        case GPIO_AF4:
            return GPIO_AF4;
        case SPI1:
        case SPI2:
        case GPIO_AF5:
            return GPIO_AF5;
        case SPI3:
        case GPIO_AF6:
            return GPIO_AF6;
        case GPIO_AF10:
            return GPIO_AF10;
        case GPIO_AF14:
            return GPIO_AF14;
    }
    return ltassert();
}

INLINE static inline void GPIO_setup_output(struct mcu_pin pin, enum gpio_output_type type)
{
    gpio_mode_setup(pin.port, GPIO_MODE_OUTPUT,
                  GPIO_PUPD_NONE, pin.pin);
    gpio_set_output_options(pin.port, type, GPIO_OSPEED_50MHZ, pin.pin);
}

INLINE static inline void GPIO_setup_output_af(struct mcu_pin pin, enum gpio_output_type type, uint32_t af)
{
    gpio_mode_setup(pin.port, GPIO_MODE_AF,
                  GPIO_PUPD_NONE, pin.pin);
    gpio_set_output_options(pin.port, type, GPIO_OSPEED_50MHZ, pin.pin);
    gpio_set_af(pin.port, _get_af(af), pin.pin);
}

INLINE static inline void GPIO_setup_input(struct mcu_pin pin, enum gpio_input_type type)
{
    int cfg = GPIO_PUPD_NONE;
    int mode = GPIO_MODE_INPUT;
    switch (type) {
        case ITYPE_ANALOG:
            mode = GPIO_MODE_ANALOG;
            break;
        case ITYPE_FLOAT:
            break;
        case ITYPE_PULLUP:
            cfg = GPIO_PUPD_PULLUP;
            break;
        case ITYPE_PULLDOWN:
            cfg = GPIO_PUPD_PULLDOWN;
            break;
    }
    gpio_mode_setup(pin.port, mode, cfg, pin.pin);
}

INLINE static inline void GPIO_setup_input_af(struct mcu_pin pin, enum gpio_input_type type, unsigned af)
{
    int cfg = GPIO_PUPD_NONE;
    switch (type) {
        case ITYPE_PULLUP:
            cfg = GPIO_PUPD_PULLUP;
            break;
        case ITYPE_PULLDOWN:
            cfg = GPIO_PUPD_PULLDOWN;
            break;
        default: break;
    }
    gpio_mode_setup(pin.port, GPIO_MODE_AF, cfg, pin.pin);
    gpio_set_af(pin.port, _get_af(af), pin.pin);
}

INLINE static inline void GPIO_pin_set(struct mcu_pin pin)
{
    gpio_set(pin.port, pin.pin);
}

INLINE static inline void GPIO_pin_clear(struct mcu_pin pin)
{
    gpio_clear(pin.port, pin.pin);
}

INLINE static inline uint16_t GPIO_pin_get(struct mcu_pin pin)
{
    return gpio_get(pin.port, pin.pin);
}

INLINE static inline void GPIO_pin_toggle(struct mcu_pin pin)
{
    gpio_toggle(pin.port, pin.pin);
}

#endif  // _DTX_STM32F2_GPIO_H_
