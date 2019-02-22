#ifndef _DEVO12_PORTS_H_
#define _DEVO12_PORTS_H_

#include "hardware.h"
//End ADC

//Power Switch configuration
#define _PWRSW_PORT               GPIOA
#define _PWRSW_PIN                GPIO4
#define _PWRSW_RCC_APB2ENR_IOPEN  RCC_APB2ENR_IOPAEN

#define _PWREN_PORT                GPIOE
#define _PWREN_PIN                 GPIO0
#define _PWREN_RCC_APB2ENR_IOPEN   RCC_APB2ENR_IOPEEN
//End Power switch configuration

#endif //_DEVO12_PORTS_H_
