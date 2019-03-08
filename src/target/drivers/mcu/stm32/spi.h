#ifndef _DTX_STM32_SPI_H_
#define _DTX_STM32_SPI_H_

#if defined(STM32F1)
    #include "f1/spi.h"
#elif defined(STM32F2)
    #include "f2/spi.h"
#endif

#endif  // _DTX_STM32_SPI_H_
