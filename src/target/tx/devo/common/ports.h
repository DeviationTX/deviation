#ifndef _PORTS_H_
#define _PORTS_H_

#ifndef EMULATOR
#include "target/drivers/mcu/stm32/gpio.h"
#endif

//SPI Flash
#ifndef SPIFLASH_TYPE
    #define SPIFLASH_TYPE SST25VFxxxB
#endif

#include "hardware.h"

#endif //_PORTS_H_

