#ifndef _DTX_STM32_DMA_H_
#define _DTX_STM32_DMA_H_

#include <libopencm3/stm32/dma.h>

#if defined(STM32F1)
    #include "f1/dma.h"
#elif defined(STM32F2)
    #include "f2/dma.h"
#endif

#endif  // _DTX_STM32_DMA_H_
