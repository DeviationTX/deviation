#ifndef _DTX_STM32_NVIC_H
#define _DTX_STM32_NVIC_H

#if defined(STM32F1)
    #include "f1/nvic.h"
#elif defined(STM32F2)
    #include "f2/nvic.h"
#endif

#endif  // _DTX_STM32_NVIC_H
