#ifndef _UNIVERSALTX_TARGET_H_
#define _UNIVERSALTX_TARGET_H_

#define PROTO_HAS_CYRF6936
#define PROTO_HAS_A7105
#define PROTO_HAS_CC2500
#define PROTO_HAS_NRF24L01

#define DISCOVERY 0 //FIXME: Disable this!

#define HAS_MULTIMOD_SUPPORT 0
#define SWITCH_ADDRESS 0xFFFFFFFF
#define HAS_CYRF_RESET 0

#ifndef _USART
    #define _USART               USART4
    #define _USART_DR            USART4_DR
    #define _USART_DMA           DMA2
    #define _USART_DMA_CHANNEL   DMA_CHANNEL5
    #define _USART_DMA_ISR       dma2_channel5_isr
    #define _USART_NVIC_DMA_CHANNEL_IRQ   NVIC_DMA2_CHANNEL5_IRQ
#endif

#endif /*_UNIVERSALTX_TARGET_H_ */
