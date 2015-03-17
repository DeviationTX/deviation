#ifndef _PORTS_H_
#define _PORTS_H_

#include <libopencm3/stm32/gpio.h>

#define PORT_mode_setup(io, mode, pullup) gpio_mode_setup(io.port, mode, pullup, io.pin)
#define PORT_pin_set(io)                  gpio_set(io.port,io.pin)
#define PORT_pin_clear(io)                gpio_clear(io.port,io.pin)
#define PORT_pin_get(io)                  gpio_get(io.port,io.pin)

static const struct mcu_pin CYRF_RESET_PIN ={0, 0};
static const struct mcu_pin AVR_RESET_PIN ={0, 0};

static const struct mcu_pin NRF24L01_CE   = {GPIOB, GPIO8};
static const struct mcu_pin NRF24L01_PAEN = {GPIOA, GPIO1};   //NOTE: This must be a comparator input since its max value is 1.8V
static const struct mcu_pin PA_TXEN       = {GPIOB, GPIO10};
static const struct mcu_pin PA_RXEN       = {GPIOB, GPIO11};
static const struct mcu_pin RF_MUXSEL1    = {GPIOA, GPIO15};
static const struct mcu_pin RF_MUXSEL2    = {GPIOB, GPIO3};
static const struct mcu_pin INPUT_CSN     = {GPIOA, GPIO8};
static const struct mcu_pin PASSTHRU_CSN  = {GPIOB, GPIO12};
static const struct mcu_pin MOSI          = {GPIOB, GPIO15};
static const struct mcu_pin MISO          = {GPIOB, GPIO14};
static const struct mcu_pin SCK           = {GPIOB, GPIO13};

static const struct mcu_pin PPM           = {GPIOA, GPIO0};


static const struct mcu_pin BT_STATE      = {GPIOA, GPIO4};
#if DISCOVERY
    //PA7 used by touch sensor
    static const struct mcu_pin BT_KEY        = {GPIOC, GPIO12};
    static const struct mcu_pin BT_TX         = {GPIOC, GPIO10};
    static const struct mcu_pin BT_RX         = {GPIOC, GPIO11};
    #define BTUART USART3
    #define BT_ISR usart3_4_isr
    #define BT_IRQ NVIC_USART3_4_IRQ
#else
    static const struct mcu_pin BT_KEY        = {GPIOA, GPIO7};
    static const struct mcu_pin BT_TX         = {GPIOA, GPIO2};
    static const struct mcu_pin BT_RX         = {GPIOA, GPIO3};
    #define BTUART USART2
    #define BT_ISR usart2_isr
    #define BT_IRQ NVIC_USART2_IRQ
#endif


static const struct mcu_pin module_enable[TX_MODULE_LAST] = {
       [CYRF6936] = {GPIOA, GPIO9},
       [A7105]    = {GPIOB, GPIO9},
       [CC2500]   = {GPIOA, GPIO10},
       [NRF24L01] = {GPIOB, GPIO6},
       [MULTIMOD] = {GPIOB, GPIO12},
       [MULTIMODCTL] = {GPIOA, GPIO8},
};

#endif //_PORTS_H_
