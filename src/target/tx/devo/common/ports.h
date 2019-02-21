#ifndef _PORTS_H_
#define _PORTS_H_

#ifndef EMULATOR
#include "target/drivers/mcu/stm32/gpio.h"
#endif

#define PORT_mode_setup(io, mode, pullup) gpio_set_mode(io.port, mode, pullup, io.pin)
#define PORT_pin_set(io)                  gpio_set(io.port,io.pin)
#define PORT_pin_clear(io)                gpio_clear(io.port,io.pin)
#define PORT_pin_get(io)                  gpio_get(io.port,io.pin)


//SPI Flash
#ifndef SPIFLASH_TYPE
    #define SPIFLASH_TYPE SST25VFxxxB
#endif

//Power switch configuration
#ifndef _PWRSW_PORT
    #define _PWRSW_PORT               GPIOA
    #define _PWRSW_PIN                GPIO3
    #define _PWRSW_RCC_APB2ENR_IOPEN  RCC_APB2ENR_IOPAEN
#endif //_PWRSW_PORT
#ifndef _PWREN_PORT
    #define _PWREN_PORT                GPIOA
    #define _PWREN_PIN                 GPIO2
    #define _PWREN_RCC_APB2ENR_IOPEN   RCC_APB2ENR_IOPAEN
#endif //_PWREN_PORT

#include "hardware.h"

#endif //_PORTS_H_

