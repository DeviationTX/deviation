#ifndef _DTX_STM32_RCC_H_
#define _DTX_STM32_RCC_H_

#if defined(STM32F1)
    #include "f1/rcc.h"
#elif defined(STM32F2)
    #include "f2/rcc.h"
#endif

#endif  // _DTX_STM32_RCC_H_
