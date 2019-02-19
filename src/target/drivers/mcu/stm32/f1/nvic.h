#ifndef _DTX_STM32F1_NVIC_H_
#define _DTX_STM32F1_NVIC_H_

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
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
        default: ltassert(); return 0;
    }
}

#endif  // _DTX_STM32F1_NVIC_H_
