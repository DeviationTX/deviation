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
#ifndef _SPI_FLASH_PORT
    #define _SPI_FLASH_PORT          1 //SPI1
    #define _SPI_FLASH_CSN_PIN       {GPIOB, GPIO2}
    #define _SPI_FLASH_SCK_PIN       {GPIOA, GPIO5}
    #define _SPI_FLASH_MISO_PIN      {GPIOA, GPIO6}
    #define _SPI_FLASH_MOSI_PIN      {GPIOA, GPIO7}
#endif
#ifndef EMULATOR
static const struct mcu_pin FLASH_CSN_PIN   = _SPI_FLASH_CSN_PIN;
static const struct mcu_pin FLASH_SCK_PIN   = _SPI_FLASH_SCK_PIN;
static const struct mcu_pin FLASH_MISO_PIN  = _SPI_FLASH_MISO_PIN;
static const struct mcu_pin FLASH_MOSI_PIN  = _SPI_FLASH_MOSI_PIN;
#endif
#ifndef SPIFLASH_TYPE
    #define SPIFLASH_TYPE SST25VFxxxB
#endif

#ifndef _SPI_PROTO_PORT
    #define _SPI_PROTO_PORT          2 //SPI2
    #define _SPI_PROTO_RST_PIN       {GPIOB, GPIO11}
    #define _SPI_PROTO_CSN_PIN       {GPIOB, GPIO12}
    #define _SPI_PROTO_SCK_PIN       {GPIOB, GPIO13}
    #define _SPI_PROTO_MISO_PIN      {GPIOB, GPIO14}
    #define _SPI_PROTO_MOSI_PIN      {GPIOB, GPIO15}
#endif
#ifndef EMULATOR
static const struct mcu_pin PROTO_RST_PIN   = _SPI_PROTO_RST_PIN;
static const struct mcu_pin PROTO_CSN_PIN   = _SPI_PROTO_CSN_PIN;
static const struct mcu_pin PROTO_SCK_PIN   = _SPI_PROTO_SCK_PIN;
static const struct mcu_pin PROTO_MISO_PIN  = _SPI_PROTO_MISO_PIN;
static const struct mcu_pin PROTO_MOSI_PIN  = _SPI_PROTO_MOSI_PIN;
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

#ifndef _SOUND_PORT
    #define _SOUND_PORT                GPIOA
    #define _SOUND_PIN                 GPIO1
    #define _SOUND_RCC_APB1ENR_TIMEN   RCC_APB1ENR_TIM2EN
    #define _SOUND_TIM_OC              TIM_OC2
    #define _SOUND_TIM                 TIM2
#endif //_SOUND_PORT

#ifndef _TOUCH_PORT
    #define _TOUCH_PORT                GPIOB
    #define _TOUCH_PIN                 GPIO0
    #define _TOUCH_IRQ_PIN             GPIO5
    #define _TOUCH_RCC_APB2ENR_IOPEN   RCC_APB2ENR_IOPBEN
    #define _TOUCH_COORDS_REVERSE      1
#endif //_TOUCH_PORT

#ifndef _PWM_PIN
    #define _PWM_PIN                   GPIO_USART1_TX    //GPIO9
    #define _PWM_EXTI                  EXTI9
    #define _PWM_TIM_OC                TIM_OC2
    #define _PWM_TIM_DIER_DMAEN        TIM_DIER_CC2DE
#endif //_PWM_PIN

#ifndef SYSCLK_TIM // System-clock timer
    #define SYSCLK_TIM 4
#endif

#ifndef FREQ_MHz
    #define FREQ_MHz 72
#endif

#include "hardware.h"

#endif //_PORTS_H_

