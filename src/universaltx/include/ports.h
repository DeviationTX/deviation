#ifndef _PORTS_H_
#define _PORTS_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>

#define  REGISTER_32(ADDRESS) (*((volatile unsigned int *)(ADDRESS)))
#define CLEAR_BIT(addr,mask) ((addr) &= ~(mask))
#define SET_BIT(addr,mask) ((addr) |= (mask))
#define ISER		REGISTER_32(NVIC_BASE + 0)

#define _BIT_SHIFT(IRQn)         (  (((uint32_t)(IRQn)       )    &  0x03) * 8 )
#define SCB_SHPR32(shpr_id)	MMIO32(SCB_BASE + 0x18 + (shpr_id))
#define NVIC_IPR32(ipr_id)	MMIO32(NVIC_BASE + 0x300 + (ipr_id))

#define NVIC_SET_PRIORITY(irqn, priority) \
	if (irqn >= NVIC_IRQ_COUNT) { \
		/* Cortex-M  system interrupts */ \
		SCB_SHPR32((irqn & 0x0C) - 4) = (SCB_SHPR32((irqn & 0x0C) - 4) & ~(0xFF << _BIT_SHIFT(irqn))) | \
                                                (priority << _BIT_SHIFT(irqn)); \
	} else { \
		NVIC_IPR32(irqn & 0xFC) = (NVIC_IPR32(irqn & 0xFC) & ~(0xFF << _BIT_SHIFT(irqn))) | \
                                                (priority << _BIT_SHIFT(irqn)); \
	}

#define PORT_mode_setup(io, mode, pullup) gpio_mode_setup(io.port, mode, pullup, io.pin)
#define PORT_pin_set(io)                  gpio_set(io.port,io.pin)
#define PORT_pin_clear(io)                gpio_clear(io.port,io.pin)
#define PORT_pin_get(io)                  gpio_get(io.port,io.pin)
#define TO_PIN(x) (1 << x)

#define PORT_pin_set_fast(io)             GPIO_BSRR(io.port) = io.pin;
#define PORT_pin_clear_fast(io)           GPIO_BRR(io.port) = io.pin;
#define PORT_pin_get_fast(io)             GPIO_IDR(io.port) & io.pin

static const struct mcu_pin CYRF_RESET_PIN ={0, 0};
static const struct mcu_pin AVR_RESET_PIN ={0, 0};

static const struct mcu_pin NRF24L01_CE   = {GPIOB, GPIO8};
static const struct mcu_pin NRF24L01_PAEN = {GPIOA, GPIO1};   //NOTE: This must be a comparator input since its max value is 1.8V
static const struct mcu_pin PA_TXEN       = {GPIOB, GPIO10};
static const struct mcu_pin PA_RXEN       = {GPIOB, GPIO11};
static const struct mcu_pin RF_MUXSEL1    = {GPIOA, GPIO15};
static const struct mcu_pin RF_MUXSEL2    = {GPIOB, GPIO3};
static const struct mcu_pin INPUT_CSN     = {GPIOA, GPIO8};
#define PASSTHRU_CSN_PORT                    GPIOB
#define PASSTHRU_CSN_PIN_NUM                 12
#define PASSTHRU_CSN_PIN                     TO_PIN(PASSTHRU_CSN_PIN_NUM)
static const struct mcu_pin PASSTHRU_CSN  = {PASSTHRU_CSN_PORT, PASSTHRU_CSN_PIN};
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
