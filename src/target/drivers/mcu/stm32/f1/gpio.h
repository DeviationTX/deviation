#ifndef _DTX_STM32F1_GPIO_H_
#define _DTX_STM32F1_GPIO_H_

#include <libopencm3/stm32/gpio.h> /* For GPIO* definitions */

#include "freq.h"

#define AF_FSMC 0xFFFFFFFF  // Unused for F1 but needs definition

enum gpio_output_type {
    OTYPE_OPENDRAIN = GPIO_CNF_OUTPUT_OPENDRAIN,
    OTYPE_PUSHPULL  = GPIO_CNF_OUTPUT_PUSHPULL,
};
enum gpio_input_type {
    ITYPE_ANALOG,
    ITYPE_FLOAT,
    ITYPE_PULLUP,
    ITYPE_PULLDOWN,
};

#define INLINE __attribute__((always_inline))

INLINE static inline void GPIO_setup_output(struct mcu_pin pin, enum gpio_output_type type)
{
    gpio_set_mode(pin.port, GPIO_MODE_OUTPUT_50_MHZ, type, pin.pin);
}

INLINE static inline void GPIO_setup_output_af(struct mcu_pin pin, enum gpio_output_type type, uint32_t af)
{
    (void)af;
    gpio_set_mode(pin.port, GPIO_MODE_OUTPUT_50_MHZ,
                  type == OTYPE_OPENDRAIN
                      ? GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN
                      : GPIO_CNF_OUTPUT_ALTFN_PUSHPULL,
                  pin.pin);
}

INLINE static inline void GPIO_setup_input(struct mcu_pin pin, enum gpio_input_type type)
{
    int cfg;
    switch (type) {
        case ITYPE_ANALOG: cfg = GPIO_CNF_INPUT_ANALOG; break;
        case ITYPE_FLOAT:  cfg = GPIO_CNF_INPUT_FLOAT; break;
        default:           cfg = GPIO_CNF_INPUT_PULL_UPDOWN; break;
    }
    gpio_set_mode(pin.port, GPIO_MODE_INPUT, cfg, pin.pin);
    if (type == ITYPE_PULLUP) {
        gpio_set(pin.port, pin.pin);
    } else if (type == ITYPE_PULLDOWN) {
        gpio_clear(pin.port, pin.pin);
    }
}

INLINE static inline void GPIO_setup_input_af(struct mcu_pin pin, enum gpio_input_type type, unsigned af)
{
    // STM32F1 does not have alt-func for inputs
    (void)af;
    GPIO_setup_input(pin, type);
}

static inline void GPIO_pin_set(struct mcu_pin pin)
{
    gpio_set(pin.port, pin.pin);
}

static inline void GPIO_pin_clear(struct mcu_pin pin)
{
    gpio_clear(pin.port, pin.pin);
}

static inline uint16_t GPIO_pin_get(struct mcu_pin pin)
{
    return gpio_get(pin.port, pin.pin);
}

INLINE static inline void GPIO_pin_toggle(struct mcu_pin pin)
{
    gpio_toggle(pin.port, pin.pin);
}

#endif  // _DTX_STM32F1_GPIO_H_
