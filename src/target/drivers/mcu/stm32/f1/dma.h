#ifndef DTX_STM32F1_DMA_H_
#define DTX_STM32F1_DMA_H_

#include <libopencm3/stm32/dma.h>

#define DMA_SxCR_DIR_PERIPHERAL_TO_MEM   (0 << 6)
#define DMA_SxCR_DIR_MEM_TO_PERIPHERAL   (1 << 6)
#define DMA_SxCR_MSIZE_16BIT DMA_CCR_MSIZE_16BIT
#define DMA_SxCR_PSIZE_16BIT DMA_CCR_PSIZE_16BIT
#define DMA_SxCR_MSIZE_8BIT DMA_CCR_MSIZE_8BIT
#define DMA_SxCR_PSIZE_8BIT DMA_CCR_PSIZE_8BIT

#define DMA_SxCR_PL_VERY_HIGH 0x01  // Unused on F1

inline static void DMA_stream_reset(struct dma_config dma)
{
    dma_channel_reset(dma.dma, dma.stream);
}

inline static void DMA_channel_select(struct dma_config dma)
{
    // Not implemented on STM32F1
    (void)dma;
}

inline static void DMA_set_dma_flow_control(struct dma_config dma)
{
    // Not implemented on STM32F1
    (void)dma;
}

inline static void DMA_set_priority(struct dma_config dma, uint32_t priority)
{
    // Not implemented on STM32F1
    (void)dma;
    (void)priority;
}

inline static void DMA_enable_direct_mode(struct dma_config dma)
{
    // Not implemented on STM32F1
    (void)dma;
}

inline static void DMA_set_transfer_mode(struct dma_config dma, uint32_t direction)
{
    if (direction == DMA_SxCR_DIR_PERIPHERAL_TO_MEM) {
        dma_set_read_from_peripheral(dma.dma, dma.stream);
    } else {
        dma_set_read_from_memory(dma.dma, dma.stream);
    }
}

inline static void DMA_disable_double_buffer_mode(struct dma_config dma)
{
    // Not implemented on STM32F1
    (void)dma;
}

inline static void DMA_enable_stream(struct dma_config dma)
{
    dma_enable_channel(dma.dma, dma.stream);
}

inline static void DMA_disable_stream(struct dma_config dma)
{
    dma_disable_channel(dma.dma, dma.stream);
}
#endif  // DTX_STM32F1_DMA_H_
