#ifndef DTX_STM32_ADC_H_
#define DTX_STM32_ADC_H_
#include <libopencm3/stm32/adc.h>

#if defined(STM32F1)
    #include "f1/adc.h"
#elif defined(STM32F2)
    #include "f2/adc.h"
#endif

#define ADC_PIN_TO_CHAN(port, pin) (\
    port == GPIOA ? ( \
        pin == GPIO0 ? 0 : \
        pin == GPIO1 ? 1 : \
        pin == GPIO2 ? 2 : \
        pin == GPIO3 ? 3 : \
        pin == GPIO4 ? 4 : \
        pin == GPIO5 ? 5 : \
        pin == GPIO6 ? 6 : \
        pin == GPIO7 ? 7 : cterror("unsupported_analog_pin")) : \
    port == GPIOB ? ( \
        pin == GPIO0 ? 8 : \
        pin == GPIO1 ? 9 : cterror("unsupported_analog_pin")) : \
    port == GPIOC ? ( \
        pin == GPIO0 ? 10 : \
        pin == GPIO1 ? 11 : \
        pin == GPIO2 ? 12 : \
        pin == GPIO3 ? 13 : \
        pin == GPIO4 ? 14 : \
        pin == GPIO5 ? 15 : cterror("unsupported_analog_pin")) : \
    port == GPIOF ? ( \
        pin == GPIO3 ? 9 : \
        pin == GPIO4 ? 14 : \
        pin == GPIO5 ? 15 : \
        pin == GPIO6 ? 4 : \
        pin == GPIO7 ? 5 : \
        pin == GPIO8 ? 6 : \
        pin == GPIO9 ? 7 : \
        pin == GPIO10 ? 8 : cterror("unsupported_analog_pin")) : \
    port == 0 ? 16 : cterror("unsupported_analog_pin"))   // Temperature is channel 16

#endif  // DTX_STM32_ADC_H_
