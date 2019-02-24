#ifndef DTX_STM32F2_DMA_H_
#define DTX_STM32F2_DMA_H_

inline static void DMA_stream_reset(struct dma_config dma)
{
    dma_stream_reset(dma.dma, dma.stream);
}

inline static void DMA_channel_select(struct dma_config dma)
{
    dma_channel_select(dma.dma, dma.stream, dma.channel);
}

inline static void DMA_set_dma_flow_control(struct dma_config dma)
{
    dma_set_dma_flow_control(dma.dma, dma.stream);
}

inline static void DMA_set_priority(struct dma_config dma, uint32_t priority)
{
    dma_set_priority(dma.dma, dma.stream, priority);
}

inline static void DMA_enable_direct_mode(struct dma_config dma)
{
    dma_enable_direct_mode(dma.dma, dma.stream);
}

inline static void DMA_set_transfer_mode(struct dma_config dma, uint32_t direction)
{
    dma_set_transfer_mode(dma.dma, dma.stream, direction);
}

inline static void DMA_disable_double_buffer_mode(struct dma_config dma)
{
    dma_disable_double_buffer_mode(dma.dma, dma.stream);
}

inline static void DMA_enable_stream(struct dma_config dma)
{
    dma_enable_stream(dma.dma, dma.stream);
}
#endif  // DTX_STM32F2_DMA_H_
