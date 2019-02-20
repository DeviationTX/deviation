#ifndef _DTX_STM32_TIMER_H_
#define _DTX_STM32_TIMER_H_

#include <libopencm3/stm32/timer.h>

INLINE static inline uint32_t TIM_OCx(unsigned channel)
{
    switch (channel) {
        case 1: return TIM_OC1;
        case 2: return TIM_OC1;
        case 3: return TIM_OC1;
        case 4: return TIM_OC1;
        default: ltassert(); return 0;
    }
}

INLINE static inline uint32_t TIM_DIER_CCxDE(unsigned channel)
{
    switch (channel) {
        case 1: return TIM_DIER_CC1DE;
        case 2: return TIM_DIER_CC1DE;
        default: ltassert(); return 0;
    }
}

#endif
