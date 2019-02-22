#ifndef _DTX_STM32F1_NVIC_H_
#define _DTX_STM32F1_NVIC_H_

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/usart.h>

static inline uint32_t get_nvic_dma_irq(struct dma_config dma)
{
    uint32_t irq;
    switch (dma.dma) {
        case DMA1:
            switch (dma.stream) {
                case DMA_CHANNEL1: irq = NVIC_DMA1_CHANNEL1_IRQ; break;
                case DMA_CHANNEL2: irq = NVIC_DMA1_CHANNEL2_IRQ; break;
                case DMA_CHANNEL3: irq = NVIC_DMA1_CHANNEL3_IRQ; break;
                case DMA_CHANNEL4: irq = NVIC_DMA1_CHANNEL4_IRQ; break;
                case DMA_CHANNEL5: irq = NVIC_DMA1_CHANNEL5_IRQ; break;
                case DMA_CHANNEL6: irq = NVIC_DMA1_CHANNEL6_IRQ; break;
                case DMA_CHANNEL7: irq = NVIC_DMA1_CHANNEL7_IRQ; break;
            }
            break;
        case DMA2:
            switch (dma.stream) {
                case DMA_CHANNEL1: irq = NVIC_DMA2_CHANNEL1_IRQ; break;
                case DMA_CHANNEL2: irq = NVIC_DMA2_CHANNEL2_IRQ; break;
                case DMA_CHANNEL3: irq = NVIC_DMA2_CHANNEL3_IRQ; break;
                case DMA_CHANNEL4: irq = NVIC_DMA2_CHANNEL4_5_IRQ; break;
                case DMA_CHANNEL5: irq = NVIC_DMA2_CHANNEL4_5_IRQ; break;
            }
            break;
    }
    return irq;
}

static inline uint32_t get_nvic_irq(uint32_t port)
{
    switch (port) {
        case USART1: return NVIC_USART1_IRQ;
        // case TIM1: return NVIC_TIM1_IRQ;
        case TIM2: return NVIC_TIM2_IRQ;
        case TIM3: return NVIC_TIM3_IRQ;
        case TIM4: return NVIC_TIM4_IRQ;
        case TIM5: return NVIC_TIM5_IRQ;
        case TIM6: return NVIC_TIM6_IRQ;
        case TIM7: return NVIC_TIM7_IRQ;
        // case TIM8: return NVIC_TIM8_IRQ;
        default: ltassert(); return 0;
    }
}

INLINE static inline uint32_t NVIC_EXTIx_IRQ(struct mcu_pin pin)
{
    switch (pin.pin) {
        case GPIO1: return NVIC_EXTI1_IRQ;
        case GPIO2: return NVIC_EXTI2_IRQ;
        case GPIO3: return NVIC_EXTI3_IRQ;
        case GPIO4: return NVIC_EXTI4_IRQ;
        case GPIO5: return NVIC_EXTI9_5_IRQ;
        case GPIO6: return NVIC_EXTI9_5_IRQ;
        case GPIO7: return NVIC_EXTI9_5_IRQ;
        case GPIO8: return NVIC_EXTI9_5_IRQ;
        case GPIO9: return NVIC_EXTI9_5_IRQ;
        case GPIO10: return NVIC_EXTI15_10_IRQ;
        case GPIO11: return NVIC_EXTI15_10_IRQ;
        case GPIO12: return NVIC_EXTI15_10_IRQ;
        case GPIO13: return NVIC_EXTI15_10_IRQ;
        case GPIO14: return NVIC_EXTI15_10_IRQ;
        case GPIO15: return NVIC_EXTI15_10_IRQ;
        default: ltassert(); return 0;
    }
}

#endif  // _DTX_STM32F1_NVIC_H_
