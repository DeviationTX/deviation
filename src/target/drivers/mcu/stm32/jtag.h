#ifndef _DTX_STM32_JTAG_H_
#define _DTX_STM32_JTAG_H_

#if defined(STM32F1)
    #include "f1/jtag.h"
#elif defined(STM32F2)
    #include "f2/jtag.h"
#endif

#endif  // _DTX_STM32_JTAG_H_

