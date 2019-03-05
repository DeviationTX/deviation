#ifndef _DTX_STM32_TIMER_H_
#define _DTX_STM32_TIMER_H_

#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>

INLINE static inline uint32_t TIM_OCx(unsigned channel)
{
    switch (channel) {
        case 1: return TIM_OC1;
        case 2: return TIM_OC2;
        case 3: return TIM_OC3;
        case 4: return TIM_OC4;
        default: return ltassert();
    }
}

INLINE static inline uint32_t TIM_OCxN(unsigned channel)
{
    switch (channel) {
        case 1: return TIM_OC1N;
        case 2: return TIM_OC2N;
        default: return ltassert();
    }
}


INLINE static inline uint32_t TIM_DIER_CCxDE(unsigned channel)
{
    switch (channel) {
        case 1: return TIM_DIER_CC1DE;
        case 2: return TIM_DIER_CC2DE;
        default: return ltassert();
    }
}

INLINE static inline uint32_t RST_TIMx(uint32_t tim)
{
    switch (tim) {
        case TIM1: return RST_TIM1;
        case TIM2: return RST_TIM2;
        case TIM3: return RST_TIM3;
        case TIM4: return RST_TIM4;
        case TIM5: return RST_TIM5;
        case TIM6: return RST_TIM6;
        case TIM7: return RST_TIM7;
        case TIM8: return RST_TIM8;
        case TIM9: return RST_TIM9;
        case TIM10: return RST_TIM10;
        case TIM11: return RST_TIM11;
        case TIM12: return RST_TIM12;
        case TIM13: return RST_TIM13;
        case TIM14: return RST_TIM14;
        default: return ltassert();
    }
}

INLINE static inline uint32_t TIM_FREQ_MHz(uint32_t tim)
{
    switch (tim) {
        case TIM1:
        case TIM8:
        case TIM9:
        case TIM10:
        case TIM11:
            return (APB2_FREQ_MHz == AHB_FREQ_MHz) ? APB2_FREQ_MHz : 2 * APB2_FREQ_MHz;
        case TIM2:
        case TIM3:
        case TIM4:
        case TIM5:
        case TIM6:
        case TIM7:
        case TIM12:
        case TIM13:
        case TIM14:
            return (APB1_FREQ_MHz == AHB_FREQ_MHz) ? APB1_FREQ_MHz : 2 * APB1_FREQ_MHz;
        default: return ltassert();
    }
}
#endif  // _DTX_STM32_TIMER_H_
