#ifndef _DTX_STM32_PWR_H_
#define _DTX_STM32_PWR_H_

#if defined(STM32F1)
    #include "f1/pwr.h"
#elif defined(STM32F2)
    #include "f2/pwr.h"
#endif

#endif  // _DTX_STM32_PWR_H_
