#ifndef _SPIPROTO_H_
#define _SPIPROTO_H_

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

u8 PROTOSPI_read3wire();
uint8_t spi_xfer8(uint32_t spi, uint8_t data);

#define PROTOSPI_pin_set(io) gpio_set(io.port,io.pin)
#define PROTOSPI_pin_clear(io) gpio_clear(io.port,io.pin)
#define spi_xfer 0xdeadbeef
#define PROTOSPI_xfer(byte) spi_xfer8(SPI2, byte)
#define PROTOSPI_mode_setup(io, mode, pullup) gpio_mode_setup(io.port, mode, pullup, io.pin)

#define _NOP()  asm volatile ("nop")

static const struct mcu_pin CYRF_RESET_PIN ={0, 0};
static const struct mcu_pin AVR_RESET_PIN ={0, 0};

static const struct mcu_pin NRF24L01_CE   = {GPIOB, GPIO8};
static const struct mcu_pin NRF24L01_PAEN = {GPIOA, GPIO1};   //NOTE: This must be a comparator input since its max value is 1.8V
static const struct mcu_pin PA_TXEN       = {GPIOB, GPIO10};
static const struct mcu_pin PA_RXEN       = {GPIOB, GPIO11};
static const struct mcu_pin RF_MUXSEL1    = {GPIOA, GPIO15};
static const struct mcu_pin RF_MUXSEL2    = {GPIOB, GPIO3};
static const struct mcu_pin INPUT_CSN     = {GPIOA, GPIO8};
static const struct mcu_pin MOSI          = {GPIOB, GPIO15};
static const struct mcu_pin MISO          = {GPIOB, GPIO14};
static const struct mcu_pin SCK           = {GPIOB, GPIO13};


static const struct mcu_pin module_enable[TX_MODULE_LAST] = {
       [CYRF6936] = {GPIOA, GPIO9},
       [A7105]    = {GPIOB, GPIO9},
       [CC2500]   = {GPIOA, GPIO10},
       [NRF24L01] = {GPIOB, GPIO6},
};
#define MODULE_ENABLE module_enable
#endif // _SPIPROTO_H_
