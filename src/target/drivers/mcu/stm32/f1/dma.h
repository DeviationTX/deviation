#ifndef DTX_STM32F1_DMA_H_
#define DTX_STM32F1_DMA_H_

#include <libopencm3/stm32/dma.h>

#define 	DMA_SxCR_DIR_PERIPHERAL_TO_MEM   (0 << 6)
#define 	DMA_SxCR_DIR_MEM_TO_PERIPHERAL   (1 << 6)

inline static void DMA_stream_channel_reset(uint32_t dma, uint32_t stream, uint32_t channel)
{
    (void)stream;
    dma_channel_reset(dma, channel);
}

inline static void DMA_channel_select(uint32_t dma, uint32_t stream, uint32_t channel)
{
    (void)stream;
    dma_enable_channel(dma, channel);
}

inline static void DMA_set_dma_flow_control(uint32_t dma, uint32_t stream)
{
    // Not implemented on STM32F1
    (void)dma;
    (void)stream;
}

inline static void DMA_set_priority(uint32_t dma, uint32_t stream, uint32_t priority)
{
    // Not implemented on STM32F1
    (void)dma;
    (void)stream;
    (void)priority;
}

inline static void DMA_enable_direct_mode(uint32_t dma, uint32_t stream)
{
    // Not implemented on STM32F1
    (void)dma;
    (void)stream;
}

inline static void DMA_set_transfer_mode(uint32_t dma, uint32_t stream, uint32_t channel, uint32_t direction)
{
    (void)stream;
    if(direction == DMA_SxCR_DIR_PERIPHERAL_TO_MEM) {
        dma_set_read_from_peripheral(dma, channel);
    } else {
        dma_set_read_from_memory(dma, channel);
    }

}

inline static void DMA_disable_double_buffer_mode(uint32_t dma, uint32_t stream)
{
    // Not implemented on STM32F1
    (void)dma;
    (void)stream;
}

inline static void DMA_enable_stream(uint32_t dma, uint32_t stream)
{
    // Not implemented on STM32F1
    (void)dma;
    (void)stream;
}
#endif  // DTX_STM32F1_DMA_H_
