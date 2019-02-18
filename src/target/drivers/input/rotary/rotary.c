#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/exti.h>

#include "common.h"

// To use interpretation of rapid rotary movement as long press
// this timeout should be less than long press timeout, 100ms
#define ROTARY_TIMEOUT 50

volatile int rotary = 0;
static u32 last_rotary_clock = 0;

void ROTARY_Init()
{
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);

    /*Rotary */
    gpio_set_mode(GPIOC, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
                  GPIO13 | GPIO14 | GPIO15);
    gpio_set(GPIOC, GPIO13 | GPIO14 | GPIO15);

    nvic_enable_irq(NVIC_EXTI15_10_IRQ);
    nvic_set_priority(NVIC_EXTI15_10_IRQ, 66); //Medium priority
    exti_select_source(EXTI13 | EXTI14, GPIOC);
    exti_set_trigger(EXTI13 | EXTI14, EXTI_TRIGGER_BOTH);
    exti_enable_request(EXTI13 | EXTI14);
}

u32 ROTARY_Scan()
{
    int last_rotary;
    u32 result = 0;

    result |= ! gpio_get(GPIOC, GPIO15) ? CHAN_ButtonMask(BUT_ENTER) : 0;

    last_rotary = rotary;
    if (last_rotary) {
        u32 rotary_clock = CLOCK_getms();
        // To prevent rotary to generate button clicks too frequently we register
        // an event in 'result' not more often than every ROTARY_TIMEOUT msec
        if (rotary_clock > last_rotary_clock) {
            result |= last_rotary > 0 ? CHAN_ButtonMask(BUT_DOWN) : CHAN_ButtonMask(BUT_UP);
            last_rotary_clock = rotary_clock + ROTARY_TIMEOUT;
        }
        rotary = 0;
    }

    return result;
}